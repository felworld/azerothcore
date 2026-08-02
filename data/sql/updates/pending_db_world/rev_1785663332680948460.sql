--
DELETE FROM `acore_string` WHERE `entry` IN (4600001, 4600002);
INSERT INTO `acore_string` (`entry`, `content_default`, `locale_koKR`, `locale_frFR`, `locale_deDE`, `locale_zhCN`, `locale_zhTW`, `locale_esES`, `locale_esMX`, `locale_ruRU`) VALUES
(4600001, '|cff00ff00{} has come online.|r', '|cff00ff00{}님이 접속했습니다.|r', '|cff00ff00{} vient de se connecter.|r', '|cff00ff00{} ist jetzt online.|r', '|cff00ff00{} 已上线。|r', '|cff00ff00{} 已上線。|r', '|cff00ff00{} se ha conectado.|r', '|cff00ff00{} se ha conectado.|r', '|cff00ff00{} в сети.|r'),
(4600002, '|cff909090{} has gone offline.|r', '|cff909090{}님이 접속을 종료했습니다.|r', '|cff909090{} vient de se déconnecter.|r', '|cff909090{} ist jetzt offline.|r', '|cff909090{} 已下线。|r', '|cff909090{} 已下線。|r', '|cff909090{} se ha desconectado.|r', '|cff909090{} se ha desconectado.|r', '|cff909090{} не в сети.|r');
