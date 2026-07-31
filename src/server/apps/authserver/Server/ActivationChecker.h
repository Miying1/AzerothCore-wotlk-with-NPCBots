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

#ifndef __ACTIVATIONCHECKER_H__
#define __ACTIVATIONCHECKER_H__

#include <string>
#include <vector>

class ActivationChecker
{
public:
    static ActivationChecker* instance();

    void Initialize();
    bool IsActivated() const;
    bool IsLocalIP(std::string const& ip) const;

private:
    ActivationChecker() = default;

    std::string LoadFile(std::string const& path);
    bool RSAVerifySignature(std::string const& keyPath, std::string const& payload, std::vector<uint8> const& signature);

    std::string GetMachineGuid();
    std::string GetMotherboardUUID();
    std::vector<std::string> GetLocalIPs();

    bool _isActivated = false;
    std::vector<std::string> _localIPs;
};

#define sActivationChecker ActivationChecker::instance()

#endif
