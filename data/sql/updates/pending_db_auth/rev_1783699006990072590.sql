-- Add RBAC permission for the .pause command.
DELETE FROM `rbac_permissions` WHERE `id` = 1000;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(1000, 'Command: pause');

DELETE FROM `rbac_linked_permissions` WHERE `id` = 197 AND `linkedId` = 1000;
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(197, 1000);
