-- Add RBAC permission for the .pause command.
DELETE FROM `rbac_permissions` WHERE `id` = 939;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(939, 'Command: pause');

DELETE FROM `rbac_linked_permissions` WHERE `id` = 197 AND `linkedId` = 939;
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(197, 939);
