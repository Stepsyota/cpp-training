#pragma once

#include <functional>
#include <string>

// Отправляет вопрос гонцу (другой поток) и возвращает его ответ.
std::string ask_oracle(std::function<std::string()> question);
