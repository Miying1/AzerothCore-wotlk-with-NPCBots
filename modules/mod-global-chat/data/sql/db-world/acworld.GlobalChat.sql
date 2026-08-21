DELETE FROM `command` WHERE name IN ('chat');

INSERT INTO `command` (`name`, `security`, `help`) VALUES 
('chat', 0, '用法：.chat $消息\n.chat on 显示全局聊天\n.chat off 隐藏全局聊天');
