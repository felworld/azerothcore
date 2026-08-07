--
DROP TABLE IF EXISTS `felworld_events`;
CREATE TABLE `felworld_events` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `time` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `guid` INT UNSIGNED NOT NULL DEFAULT 0,
    `event_type` VARCHAR(64) NOT NULL,
    `details` TEXT,
    PRIMARY KEY (`id`),
    KEY `idx_felworld_events_guid_time` (`guid`, `time`),
    KEY `idx_felworld_events_type_time` (`event_type`, `time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
