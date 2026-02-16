/*
 * config.hpp
 *
 * @brief Defines the structures that will be representing the configuration for the webserv
 * modules.
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

typedef struct Location_Config {
} Location_Config;

typedef struct Server_Config {
} Server_Config;

typedef struct HTTP_Config {
} HTTP_Config;

typedef struct Config {
    HTTP_Config http;
} Config;

#endif
