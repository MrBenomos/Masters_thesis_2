#pragma once
#include <exception>
#include <string>
#include <QString>

class STR
{
   std::string m_str;

public:
   STR(const char* str_) : m_str(str_) {}
   STR(const std::string& str_) : m_str(str_) {}
   STR(const QString& str_) : m_str(str_.toUtf8().constData()) {}

   operator std::string() const { return m_str; }
};

class CException : public std::exception
{
   std::string m_message;
   std::string m_title;
   std::string m_location;

public:
   CException() = delete;

   CException(const STR& message_, const STR& title_ = "", const STR& location_ = "")
      : std::exception(),  // Не передаем строку в базовый класс
      m_message(message_), m_title(title_), m_location(location_)
   {
   }

   ~CException() noexcept = default;

   const char* what() const noexcept override { return m_message.c_str(); }
   const char* message() const noexcept { return what(); }
   const char* title() const noexcept { return m_title.c_str(); }
   const char* location() const noexcept { return m_location.c_str(); }

   void title(const STR& newTitle_) { m_title = newTitle_; }
   void location(const STR& newLocation_) { m_location = newLocation_; }
};
