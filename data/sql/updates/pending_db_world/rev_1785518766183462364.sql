--
DELETE FROM `command` WHERE `name` = 'lookup player character';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('lookup player character', 2, 'Syntax: .lookup player character [$name] ($limit)\r\n\r\nLists all characters on the account that $name belongs to (the selected character if no name is given), with an optional $limit of results.');
