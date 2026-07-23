--
DELETE FROM `command` WHERE `name` = 'modify xp';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('modify xp', 2, 'Syntax: .modify xp [#rate]\r\n\r\nSet the XP gain rate of the selected player to #rate, a multiplier applied to all XP sources (1 = normal, 0 = no XP). If no player is selected, modify your rate. Without #rate, shows the current rate. Resets on logout.');
