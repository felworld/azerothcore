-- Add the .pause command (freeze gameplay while keeping chat available).
DELETE FROM `command` WHERE `name` = 'pause';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('pause', 2, 'Syntax: .pause [on|off]\nPause or resume all gameplay (maps, creatures, battlegrounds, playerbot AI). Without an argument, toggles the current state. Chat and GM commands keep working while paused.');
