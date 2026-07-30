/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ActivationChecker.h"
#include "Base64.h"
#include "Log.h"
#include <filesystem>
#include <fstream>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <sstream>

#ifdef _WIN32
#include <comdef.h>
#include <iphlpapi.h>
#include <wbemidl.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

namespace fs = std::filesystem;

ActivationChecker* ActivationChecker::instance()
{
    static ActivationChecker instance;
    return &instance;
}

void ActivationChecker::Initialize()
{
    // Collect local IPv4 addresses for later comparison
    _localIPs = GetLocalIPs();

    // Default: not activated
    _isActivated = false;

    // Determine data directory relative to working directory
    fs::path dataDir = fs::current_path() / "data";

    fs::path keyPath   = dataDir / "rsa_public.pem";
    fs::path activePath = dataDir / "activate.key";

    // If either file does not exist, silently remain unactivated
    if (!fs::exists(activePath) || !fs::exists(keyPath))
        return;

    // Read the encrypted activation key
    std::string encryptedData;
    try
    {
        encryptedData = LoadFile(activePath.string());
    }
    catch (std::exception const&)
    {
        return;
    }

    if (encryptedData.empty())
        return;

    // Trim whitespace/newlines from the file content
    while (!encryptedData.empty() && (encryptedData.back() == '\n' || encryptedData.back() == '\r' || encryptedData.back() == ' '))
        encryptedData.pop_back();

    // Decrypt
    std::string decrypted;
    try
    {
        decrypted = RSADecrypt(keyPath.string(), encryptedData);
    }
    catch (std::exception const& e)
    {
        LOG_WARN("server.authserver", "[Activation] RSA decryption failed: {}", e.what());
        return;
    }

    if (decrypted.empty())
        return;

    // Parse format: code:MachineGuid|motherboard_uuid
    // The code (before colon) is arbitrary; the part after colon is "MachineGuid|UUID"
    size_t colonPos = decrypted.find(':');
    if (colonPos == std::string::npos)
    {
        LOG_WARN("server.authserver", "[Activation] Invalid activate key format (missing colon separator)");
        return;
    }

    std::string idsPart = decrypted.substr(colonPos + 1);

    size_t pipePos = idsPart.find('|');
    if (pipePos == std::string::npos)
    {
        LOG_WARN("server.authserver", "[Activation] Invalid activate key format (missing pipe separator)");
        return;
    }

    std::string keyMachineGuid = idsPart.substr(0, pipePos);
    std::string keyMBUUID      = idsPart.substr(pipePos + 1);

    // Get local machine identifiers
    std::string localMachineGuid = GetMachineGuid();
    std::string localMBUUID      = GetMotherboardUUID();

    // Check if any identifier matches
    bool guidMatch = !keyMachineGuid.empty() && !localMachineGuid.empty()
                     && (keyMachineGuid == localMachineGuid);
    bool uuidMatch = !keyMBUUID.empty() && !localMBUUID.empty()
                     && (keyMBUUID == localMBUUID);

    if (guidMatch || uuidMatch)
    {
        _isActivated = true;
        LOG_INFO("server.authserver", "[Activation] Server activated successfully (MachineGuid match: {}, UUID match: {})", guidMatch, uuidMatch);
    }
}

bool ActivationChecker::IsActivated() const
{
    return _isActivated;
}

bool ActivationChecker::IsLocalIP(std::string const& ip) const
{
    // Always allow IPv4 loopback
    if (ip == "127.0.0.1")
        return true;

    // Check against collected local IPv4 addresses
    for (std::string const& localIP : _localIPs)
    {
        if (ip == localIP)
            return true;
    }

    return false;
}

std::string ActivationChecker::LoadFile(std::string const& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string ActivationChecker::RSADecrypt(std::string const& keyPath, std::string const& encryptedData)
{
    // Load the PEM key (private key for RSA decryption)
    std::string pemData;
    try
    {
        pemData = LoadFile(keyPath);
    }
    catch (std::exception const& e)
    {
        throw std::runtime_error(std::string("Failed to read key file: ") + e.what());
    }

    BIO* bio = BIO_new_mem_buf(pemData.data(), static_cast<int>(pemData.size()));
    if (!bio)
        throw std::runtime_error("BIO_new_mem_buf failed");

    RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    if (!rsa)
    {
        // Try loading as public key
        BIO_reset(bio);
        rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
        if (!rsa)
        {
            BIO_reset(bio);
            rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
        }
        BIO_free(bio);
        if (!rsa)
            throw std::runtime_error("PEM_read_bio_RSAPrivateKey / PEM_read_bio_RSA_PUBKEY failed: key file is not a valid RSA key");
    }
    else
    {
        BIO_free(bio);
    }

    // Determine key size
    int keySize = RSA_size(rsa);

    // Base64 decode the encrypted data
    Optional<std::vector<uint8>> decoded = Acore::Encoding::Base64::Decode(encryptedData);
    if (!decoded)
    {
        RSA_free(rsa);
        throw std::runtime_error("Base64 decode of activate.key failed");
    }

    if (decoded->empty())
    {
        RSA_free(rsa);
        throw std::runtime_error("Decoded activate.key data is empty");
    }

    // Decrypt with RSA private key (PKCS1v15 padding - matches crypto.go Sign/encrypt approach)
    std::vector<uint8> decrypted(keySize);
    int decryptedLen = RSA_private_decrypt(
        static_cast<int>(decoded->size()),
        decoded->data(),
        decrypted.data(),
        rsa,
        RSA_PKCS1_PADDING
    );

    RSA_free(rsa);

    if (decryptedLen == -1)
    {
        char errBuf[256];
        ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));
        throw std::runtime_error(std::string("RSA_private_decrypt failed: ") + errBuf);
    }

    return std::string(reinterpret_cast<char*>(decrypted.data()), static_cast<size_t>(decryptedLen));
}

std::string ActivationChecker::GetMachineGuid()
{
#ifdef _WIN32
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey);

    if (result != ERROR_SUCCESS)
        return "";

    char value[256];
    DWORD size = sizeof(value);
    DWORD type = 0;
    result = RegQueryValueExA(hKey, "MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(value), &size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return "";

    // size includes the null terminator for REG_SZ
    return std::string(value, size > 0 ? size - 1 : 0);
#else
    return "";
#endif
}

std::string ActivationChecker::GetMotherboardUUID()
{
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool bNeedUninit = SUCCEEDED(hr);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return "";

    std::string result;

    IWbemLocator* pLoc = nullptr;
    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));

    if (SUCCEEDED(hr) && pLoc)
    {
        IWbemServices* pSvc = nullptr;
        hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
            0, nullptr, nullptr, &pSvc);

        if (SUCCEEDED(hr) && pSvc)
        {
            hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

            if (SUCCEEDED(hr))
            {
                IEnumWbemClassObject* pEnumerator = nullptr;
                hr = pSvc->ExecQuery(
                    _bstr_t("WQL"),
                    _bstr_t("SELECT UUID FROM Win32_ComputerSystemProduct"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    nullptr, &pEnumerator);

                if (SUCCEEDED(hr) && pEnumerator)
                {
                    IWbemClassObject* pclsObj = nullptr;
                    ULONG uReturn = 0;
                    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

                    if (SUCCEEDED(hr) && uReturn > 0 && pclsObj)
                    {
                        VARIANT vtProp;
                        VariantInit(&vtProp);
                        hr = pclsObj->Get(L"UUID", 0, &vtProp, 0, 0);
                        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
                        {
                            _bstr_t bstr(vtProp.bstrVal);
                            result = static_cast<const char*>(bstr);
                        }
                        VariantClear(&vtProp);
                        pclsObj->Release();
                    }
                    pEnumerator->Release();
                }
            }
            pSvc->Release();
        }
        pLoc->Release();
    }

    if (bNeedUninit)
        CoUninitialize();
    return result;
#else
    return "";
#endif
}

std::vector<std::string> ActivationChecker::GetLocalIPs()
{
    std::vector<std::string> ips;

    // Always include loopback
    ips.push_back("127.0.0.1");

#ifdef _WIN32
    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());

    ULONG ret = GetAdaptersAddresses(AF_INET,
        GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
        nullptr, pAddresses, &bufLen);

    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        buf.resize(bufLen);
        pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        ret = GetAdaptersAddresses(AF_INET,
            GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, pAddresses, &bufLen);
    }

    if (ret == NO_ERROR)
    {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next)
        {
            // Skip loopback adapters (127.0.0.1 already included)
            if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                continue;

            // Skip non-operational adapters
            if (pCurr->OperStatus != IfOperStatusUp)
                continue;

            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                 pUnicast; pUnicast = pUnicast->Next)
            {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                {
                    char ipStr[INET_ADDRSTRLEN];
                    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                    inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
                    ips.push_back(ipStr);
                }
            }
        }
    }
#endif

    return ips;
}
