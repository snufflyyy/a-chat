#pragma once

void a_chat_log_error(const char* message);
void a_chat_log_error_gai_strerror(const char* message, int status);
void a_chat_log_error_errno(const char* message);
void a_chat_log_warning_errno(const char* message);
void a_chat_log_info(const char* message);
