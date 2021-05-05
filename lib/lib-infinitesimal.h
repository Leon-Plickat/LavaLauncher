#ifndef _INFINITESIMAL_H
#define _INFINITESIMAL_H

#include<stdbool.h>
#include<stdint.h>

/**
 * Section callback. Gets called when a section (like "[section]") is encountered.
 * The section name passed to the callback includes the square brackets and must
 * not be modified.
 * 
 * Additionally, both the line number and the user data are passed to the
 * callback function.
 * 
 * The callback function should return false when the section name is not valid
 * for some reason, otherwise true. If false is returned, infinitesimal_parse_file()
 * aborts and returns false.
 */
typedef bool (*infinitesimal_section_callback_t)(void *user_data, uint32_t line, const char *section);

/**
 * Assignment callback. Gets called when an assignment (like "variable = value")
 * is encountered. The variable name and the value string must not be modified.
 * 
 * Additionally, both the line number and the user data are passed to the
 * callback function.
 * 
 * This function should return false when the assignment is not valid for some
 * reason (like when the variable does not exist, or does not support the value),
 * otherwise true. If false is returned, infinitesimal_parse_file() aborts and
 * returns false.
 */
typedef bool (*infinitesimal_assign_callback_t)(void *user_data, uint32_t line, const char *variable, const char *value);

/**
 * Error message callback. Gets called when an error is encountered. Passes the
 * line number and a short error message so lib-infinitesimal errors can be
 * neatly integrated in the users log mechanism.
 */
typedef void (*infinitesimal_error_message_callback_t)(void *user_data, uint32_t line, const char *message);

/**
 * Tries parsing a files contents, calling user callback functions when a header,
 * assignment or error is encountered. Returns true when file was parsed
 * without errors, otherwise false. The user data void pointer is passed to the
 * callback function.
 */
bool infinitesimal_parse_file (FILE *file, void *user_data,
		infinitesimal_section_callback_t section_callback,
		infinitesimal_assign_callback_t assign_callback,
		infinitesimal_error_message_callback_t error_message_callback);

#endif

