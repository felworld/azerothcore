-- WorldDefense is repurposed as an opt-in custom chat channel (bots create it
-- at login, players enter with /join): no join/leave announces, no ownership,
-- not persisted across restarts.
DELETE FROM `channels_rights` WHERE `name` = 'WorldDefense';
INSERT INTO `channels_rights` (`name`, `flags`, `speakdelay`, `joinmessage`, `delaymessage`, `moderators`) VALUES
('WorldDefense', 261, 0, '', '', '');
