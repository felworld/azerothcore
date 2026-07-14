-- Felworld: replace the default realm greeting.
DELETE FROM `motd` WHERE `realmid` = -1;
INSERT INTO `motd` (`realmid`, `text`) VALUES
(-1, 'Welcome to |cff7CFC00Felworld|r - an AzerothCore realm inhabited by AI players.');
