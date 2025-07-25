/// \file nanodbc.h The entirety of nanodbc can be found within this file and nanodbc.cpp.

/// \mainpage
///
/// \section synopsis Synopsis
/// This library provides a wrapper API for the native ODBC API. It aims to do everything ODBC does,
/// but with a \b much nicer interface. Anything it doesn't (yet) do can be done by retrieving the
/// native ODBC handles and dropping down to straight ODBC C API code.
/// For more propaganda, please see the <a href="http://nanodbc.io/">project
/// homepage</a>.
///
/// \section toc Table of Contents
/// - \ref license "License"
/// - \ref credits "Credits"
/// - Source level documentation:
///     - \ref nanodbc "nanodbc namespace"
///     - \ref exceptions
///     - \ref utility
///     - \ref mainc
///     - \ref mainf
///     - \ref binding
///     - \ref bind_multi
///     - \ref bind_strings
///
/// \section license License
/// <div class="license">
/// Copyright (C) 2013 lexicalunit <lexicalunit@lexicalunit.com>
/// Copyright (C) 2015-2022 Mateusz Loskot <mateusz@loskot.net>
///
/// The MIT License
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
/// </div>
///
/// \section credits Credits
/// <div class="license">
/// Much of the code in this file was originally derived from TinyODBC.
/// TinyODBC is hosted at http://code.google.com/p/tiodbc/
/// Copyright (C) 2008 SqUe squarious@gmail.com
/// License: The MIT License
///
/// The idea for using RAII for transactions was inspired by SimpleDB: C++ ODBC database API,
/// however the code in nanodbc is original and not derived from SimpleDB. Therefore
/// the LGPL license under which SimpleDB is distributed does NOT apply to nanodbc.
/// SimpleDB is hosted at http://simpledb.sourceforge.net
/// Copyright (C) 2006 Eminence Technology Pty Ltd
/// Copyright (C) 2008-2010,2012 Russell Kliese russell@kliese.id.au
/// License: GNU Lesser General Public version 2.1
///
/// Some improvements and features are based on The Python ODBC Library.
/// The Python ODBC Library is hosted at http://code.google.com/p/pyodbc/
/// License: The MIT License
///
/// Implementation of column binding inspired by Nick E. Geht's source code posted to on CodeGuru.
/// GSODBC hosted at http://www.codeguru.com/mfc_database/gsodbc.html
/// Copyright (C) 2002 Nick E. Geht
/// License: Perpetual license to reproduce, distribute, adapt, perform, display, and sublicense.
/// See http://www.codeguru.com/submission-guidelines.php for details.
/// </div>

#ifndef NANODBC_NANODBC_H
#define NANODBC_NANODBC_H

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define NANODBC_HAS_STD_STRING_VIEW
#include <optional>
#define NANODBC_HAS_STD_OPTIONAL
#include <variant>
#define NANODBC_HAS_STD_VARIANT
#endif

/// \brief The entirety of nanodbc can be found within this one namespace.
///
/// \note This library does not make any exception safety guarantees, but should work just fine with
///       a threading enabled ODBC driver. If you want to use nanodbc objects in threads I recommend
///       each thread keep their own connection to the database. Otherwise you must synchronize any
///       access to nanodbc objects.
namespace nanodbc
{

// clang-format off
//  .d8888b.                     .d888 d8b                                   888    d8b
// d88P  Y88b                   d88P"  Y8P                                   888    Y8P
// 888    888                   888                                          888
// 888         .d88b.  88888b.  888888 888  .d88b.  888  888 888d888 8888b.  888888 888  .d88b.  88888b.
// 888        d88""88b 888 "88b 888    888 d88P"88b 888  888 888P"      "88b 888    888 d88""88b 888 "88b
// 888    888 888  888 888  888 888    888 888  888 888  888 888    .d888888 888    888 888  888 888  888
// Y88b  d88P Y88..88P 888  888 888    888 Y88b 888 Y88b 888 888    888  888 Y88b.  888 Y88..88P 888  888
//  "Y8888P"   "Y88P"  888  888 888    888  "Y88888  "Y88888 888    "Y888888  "Y888 888  "Y88P"  888  888
//                                              888
//                                         Y8b d88P
//                                          "Y88P"
// MARK: Configuration -
// clang-format on

/// \addtogroup macros Macros
/// \brief Configuration and utility macros that nanodbc uses, can be overriden by users.
///
/// @{
#ifdef DOXYGEN

/// \def NANODBC_THROW_NO_SOURCE_LOCATION
/// \brief Configures \c nanodbc::database_error message
///
/// If defined, removes source file name and line number from \c nanodbc::database_error message
/// By default, nanodbc includes source location of exception in the error message.
#define NANODBC_THROW_NO_SOURCE_LOCATION 1

/// \def NANODBC_ASSERT(expression)
/// \brief Assertion.
///
/// By default, nanodbc uses C \c assert() for internal assertions.
/// User can override it by defining \c NANODBC_ASSERT(expr) macro
/// in the nanodbc.h file and customizing it as desired,
/// before building the library.
///
/// \code{.cpp}
/// #ifdef _DEBUG
///     #include <crtdbg.h>
///     #define NANODBC_ASSERT _ASSERTE
/// #endif
/// \endcode
#define NANODBC_ASSERT(expression) assert(expression)

#endif
/// @}

// You must explicitly request Unicode support by defining NANODBC_ENABLE_UNICODE at compile time.
#ifndef DOXYGEN
#ifdef NANODBC_ENABLE_UNICODE
#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
#define NANODBC_TEXT(s) U##s
typedef std::u32string string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::u32string_view string_view;
#endif
#else
#ifdef _MSC_VER
typedef std::wstring string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::wstring_view string_view;
#endif
#define NANODBC_TEXT(s) L##s
#else
typedef std::u16string string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::u16string_view string_view;
#endif
#define NANODBC_TEXT(s) u##s
#endif
#endif
#else
typedef std::string string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::string_view string_view;
#endif
#define NANODBC_TEXT(s) s
#endif

#ifdef NANODBC_USE_IODBC_WIDE_STRINGS
typedef std::u32string wide_string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::u32string_view wide_string_view;
#endif
#else
#ifdef _MSC_VER
typedef std::wstring wide_string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::wstring_view wide_string_view;
#endif
#else
typedef std::u16string wide_string;
#ifdef NANODBC_HAS_STD_STRING_VIEW
typedef std::u16string_view wide_string_view;
#endif
#endif
#endif

typedef wide_string::value_type wide_char_t;

#if defined(_WIN64)
// LLP64 machine: Windows
typedef std::int64_t null_type;
#elif !defined(_WIN64) && defined(__LP64__)
// LP64 machine: OS X or Linux
typedef long null_type;
#else
// 32-bit machine
typedef long null_type;
#endif
#else
/// \def NANODBC_TEXT(s)
/// \brief Creates a string literal of the type corresponding to `nanodbc::string`.
///
/// By default, the macro maps to an unprefixed string literal.
/// If building with options NANODBC_ENABLE_UNICODE=ON and
/// NANODBC_USE_IODBC_WIDE_STRINGS=ON specified, then it prefixes a literal with U"...".
/// If only NANODBC_ENABLE_UNICODE=ON is specified, then:
///   * If building with Visual Studio, then the macro prefixes a literal with L"...".
///   * Otherwise, it prefixes a literal with u"...".
#define NANODBC_TEXT(s) s

/// \c string will be \c std::u16string or \c std::32string if \c NANODBC_ENABLE_UNICODE
/// defined.
///
/// Otherwise it will be \c std::string.
typedef unspecified - type string;
/// \c null_type will be \c int64_t for 64-bit compilations, otherwise \c long.
typedef unspecified - type null_type;
#endif

#if __cplusplus >= 201402L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
// [[deprecated]] is only available in C++14
#define NANODBC_DEPRECATED [[deprecated]]
#else
#ifdef __GNUC__
#define NANODBC_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define NANODBC_DEPRECATED __declspec(deprecated)
#else
#define NANODBC_DEPRECATED
#endif
#endif

// forward declare
#ifndef NANODBC_DISABLE_MSSQL_TVP
class table_valued_parameter;
#endif
class statement;
class connection;
class transaction;
class catalog;
class result;

// clang-format off
// 8888888888                                      888    888                        888 888 d8b
// 888                                             888    888                        888 888 Y8P
// 888                                             888    888                        888 888
// 8888888    888d888 888d888 .d88b.  888d888      8888888888  8888b.  88888b.   .d88888 888 888 88888b.   .d88b.
// 888        888P"   888P"  d88""88b 888P"        888    888     "88b 888 "88b d88" 888 888 888 888 "88b d88P"88b
// 888        888     888    888  888 888          888    888 .d888888 888  888 888  888 888 888 888  888 888  888
// 888        888     888    Y88..88P 888          888    888 888  888 888  888 Y88b 888 888 888 888  888 Y88b 888
// 8888888888 888     888     "Y88P"  888          888    888 "Y888888 888  888  "Y88888 888 888 888  888  "Y88888
//                                                                                                             888
//                                                                                                        Y8b d88P
//                                                                                                         "Y88P"
// MARK: Error Handling -
// clang-format on

/// \addtogroup exceptions Exception types
/// \brief Possible error conditions.
///
/// Specific errors such as \c type_incompatible_error, \c null_access_error, and
/// \c index_range_error can arise from improper use of the nanodbc library. The general
/// \c database_error is for all other situations in which the ODBC driver or C API reports an error
/// condition. The explanatory string for database_error will, if possible, contain a diagnostic
/// message obtained from \c SQLGetDiagRec().
/// @{

/// \brief Type incompatible.
/// \see exceptions
class type_incompatible_error : public std::runtime_error
{
public:
    type_incompatible_error();
    char const* what() const noexcept override;
};

/// \brief Accessed null data.
/// \see exceptions
class null_access_error : public std::runtime_error
{
public:
    null_access_error();
    char const* what() const noexcept override;
};

/// \brief Index out of range.
/// \see exceptions
class index_range_error : public std::runtime_error
{
public:
    index_range_error();
    char const* what() const noexcept override;
};

/// \brief Programming logic error.
/// \see exceptions
class programming_error : public std::runtime_error
{
public:
    explicit programming_error(std::string const& info);
    char const* what() const noexcept override;
};

/// \brief General database error.
/// \see exceptions
class database_error : public std::runtime_error
{
public:
    /// \brief Creates runtime_error with message about last ODBC error.
    /// \param handle The native ODBC statement or connection handle.
    /// \param handle_type The native ODBC handle type code for the given handle.
    /// \param info Additional info that will be appended to the beginning of the error message.
    database_error(void* handle, short handle_type, std::string const& info = "");
    char const* what() const noexcept override;
    long native() const noexcept;
    std::string const& state() const noexcept;

private:
    long native_error;
    std::string sql_state;
    std::string message;
};

/// @}

// clang-format off
// 888     888 888    d8b 888 d8b 888    d8b
// 888     888 888    Y8P 888 Y8P 888    Y8P
// 888     888 888        888     888
// 888     888 888888 888 888 888 888888 888  .d88b.  .d8888b
// 888     888 888    888 888 888 888    888 d8P  Y8b 88K
// 888     888 888    888 888 888 888    888 88888888 "Y8888b.
// Y88b. .d88P Y88b.  888 888 888 Y88b.  888 Y8b.          X88
//  "Y88888P"   "Y888 888 888 888  "Y888 888  "Y8888   88888P'
// MARK: Utilities -
// clang-format on

/// \addtogroup utility Utilities
/// \brief Additional nanodbc utility classes and functions.
///
/// \{

/// \brief A type capturing parameter array length as well as
/// number of rows in a rowset of a result.
struct batch_ops
{
    long parameter_array_length;
    long rowset_size;

    batch_ops() noexcept
        : parameter_array_length(-1L)
        , rowset_size(-1L){};
    batch_ops(const long all_length) noexcept
        : parameter_array_length(all_length)
        , rowset_size(all_length){};
};

/// \brief A type for representing date data.
struct date
{
    std::int16_t year;  ///< Year [0-inf).
    std::int16_t month; ///< Month of the year [1-12].
    std::int16_t day;   ///< Day of the month [1-31].
};

/// \brief A type for representing time data.
struct time
{
    std::int16_t hour; ///< Hours since midnight [0-23].
    std::int16_t min;  ///< Minutes after the hour [0-59].
    std::int16_t sec;  ///< Seconds after the minute.
};

/// \brief A type for representing timestamp data.
struct timestamp
{
    std::int16_t year;  ///< Year [0-inf).
    std::int16_t month; ///< Month of the year [1-12].
    std::int16_t day;   ///< Day of the month [1-31].
    std::int16_t hour;  ///< Hours since midnight [0-23].
    std::int16_t min;   ///< Minutes after the hour [0-59].
    std::int16_t sec;   ///< Seconds after the minute.
    std::int32_t fract; ///< Fractional seconds.
};

#ifdef NANODBC_HAS_STD_VARIANT
/// \brief A class representing a connection or a statement attribute.
///
/// Callers should create attributes using the 3 argument constructor.
/// First argument is the Attribute argument to the ODBC API call -
//  `SQLSetConnectAttr`, or `SQLSetStmtAttr`.  The second is the StringLength,
//  and the third is used to inform the ValuePtr argument.  This argument,
/// a std::variant, is a type safe union of std::vector<uint8_t> ( binary
/// buffer payloads ), nanodbc::string ( string payloads ), or std::(u)intptr_t,
/// for both u/int payloads, as well as pointers to more generic buffers.
///
/// See https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlsetconnectattr-function
class attribute
{
public:
#ifdef NANODBC_ENABLE_UNICODE
    typedef std::variant<std::vector<uint8_t>, string, std::string, std::intptr_t, std::uintptr_t>
        variant;
#else
    typedef std::variant<std::vector<uint8_t>, string, std::intptr_t, std::uintptr_t> variant;
#endif
    attribute() = delete;
    attribute& operator=(attribute const&) = delete;
    attribute(attribute const& other) noexcept;
    attribute(long const& attribute, long const& string_length, variant const& resource) noexcept;

protected:
    void extractValuePtr();

    long attribute_;
    long string_length_;
    variant resource_;
    void* value_ptr_;
};
#else
class attribute
{
public:
    attribute(long const& attribute, long const& string_length, std::uintptr_t value_ptr) noexcept
        : attribute_(attribute)
        , string_length_(string_length)
        , value_ptr_((void*)value_ptr){};

protected:
    long attribute_;
    long string_length_;
    void* value_ptr_;
};
#endif

/// \brief A type trait for testing if a type is a std::basic_string compatible with the current
/// nanodbc configuration
template <typename T>
using is_string = std::integral_constant<
    bool,
    std::is_same<typename std::decay<T>::type, std::string>::value ||
        std::is_same<typename std::decay<T>::type, wide_string>::value
#ifdef NANODBC_HAS_STD_STRING_VIEW
        || std::is_same<typename std::decay<T>::type, std::string_view>::value ||
        std::is_same<typename std::decay<T>::type, wide_string_view>::value
#endif
    >;

/// \brief A type trait for testing if a type is a character compatible with the current nanodbc
/// configuration
template <typename T>
using is_character = std::integral_constant<
    bool,
    std::is_same<typename std::decay<T>::type, std::string::value_type>::value ||
        std::is_same<typename std::decay<T>::type, wide_char_t>::value>;

template <typename T>
using enable_if_string = typename std::enable_if<is_string<T>::value>::type;

template <typename T>
using enable_if_character = typename std::enable_if<is_character<T>::value>::type;

/// \}

/// \addtogroup mainc Main classes
/// \brief Main nanodbc classes.
///
/// @{

// clang-format off
// 88888888888                                                  888    d8b
//     888                                                      888    Y8P
//     888                                                      888
//     888  888d888 8888b.  88888b.  .d8888b   8888b.   .d8888b 888888 888  .d88b.  88888b.
//     888  888P"      "88b 888 "88b 88K          "88b d88P"    888    888 d88""88b 888 "88b
//     888  888    .d888888 888  888 "Y8888b. .d888888 888      888    888 888  888 888  888
//     888  888    888  888 888  888      X88 888  888 Y88b.    Y88b.  888 Y88..88P 888  888
//     888  888    "Y888888 888  888  88888P' "Y888888  "Y8888P  "Y888 888  "Y88P"  888  888
// MARK: Transaction -
// clang-format on

/// \brief A resource for managing transaction commits and rollbacks.
/// \attention You will want to use transactions if you are doing batch operations because it will
///            prevent auto commits from occurring after each individual operation is executed.
class transaction
{
public:
    /// \brief Begin a transaction on the given connection object.
    /// \post Operations that modify the database must now be committed before taking effect.
    /// \throws database_error
    explicit transaction(const class connection& conn);

    /// Copy constructor.
    transaction(const transaction& rhs);

    /// Move constructor.
    transaction(transaction&& rhs) noexcept;

    /// Assignment.
    transaction& operator=(transaction rhs);

    /// Member swap.
    void swap(transaction& rhs) noexcept;

    /// \brief If this transaction has not been committed, it will rollback any modifying ops.
    ~transaction() noexcept;

    /// \brief Commits transaction immediately.
    /// \throws database_error
    void commit();

    /// \brief Marks this transaction for rollback.
    void rollback() noexcept;

    /// Returns the connection object.
    class connection& connection();

    /// Returns the connection object.
    const class connection& connection() const;

    /// Returns the connection object.
    operator class connection &();

    /// Returns the connection object.
    operator const class connection &() const;

private:
    class transaction_impl;
    friend class nanodbc::connection;

private:
    std::shared_ptr<transaction_impl> impl_;
};

// clang-format off
// 888b     d888  .d8888b.   .d8888b.   .d88888b.  888                       88888888888 888     888 8888888b.
// 8888b   d8888 d88P  Y88b d88P  Y88b d88P" "Y88b 888                           888     888     888 888   Y88b
// 88888b.d88888 Y88b.      Y88b.      888     888 888                           888     888     888 888    888
// 888Y88888P888  "Y888b.    "Y888b.   888     888 888                           888     Y88b   d88P 888   d88P
// 888 Y888P 888     "Y88b.     "Y88b. 888     888 888                           888      Y88b d88P  8888888P"
// 888  Y8P  888       "888       "888 888 Y8b 888 888           888888          888       Y88o88P   888
// 888   "   888 Y88b  d88P Y88b  d88P Y88b.Y8b88P 888                           888        Y888P    888
// 888       888  "Y8888P"   "Y8888P"   "Y888888"  88888888                      888         Y8P     888
//                                            Y8b
// MARK: MSSQL - TVP (Table Valued Parameters) -
// clang-format on
#ifndef NANODBC_DISABLE_MSSQL_TVP
/// \brief Support for table-valued parameter
class table_valued_parameter
{
public:
    table_valued_parameter();
    table_valued_parameter(const table_valued_parameter& rhs);
    table_valued_parameter(table_valued_parameter&& rhs) noexcept;
    table_valued_parameter(statement& stmt, short param_index, size_t row_count);

    ~table_valued_parameter() noexcept;

    void open(statement& stmt, short param_index, std::size_t row_count);
    void close();

    /// \addtogroup bind_multi Binding multiple non-string values
    /// \brief Binds given values to given parameter placeholder number in the prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark) placeholders,
    /// this is how you bind values to them.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// It is possible to use these functions for batch operations.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param values Values to substitute into placeholder.
    /// \param batch_size The number of values being bound. batch_size should greater or equal than
    /// row_count
    /// \param null_sentry Value which should represent a null value.
    /// \param nulls Flags for values that should be set to a null value.
    /// \param param_direction ODBC parameter direction.
    /// \throws database_error
    ///
    /// @{

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(short param_index, T const* values, std::size_t batch_size);

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(short param_index, T const* values, std::size_t batch_size, T const* null_sentry);

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(short param_index, T const* values, std::size_t batch_size, bool const* nulls);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void bind(short param_index, std::vector<std::vector<uint8_t>> const& values);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void
    bind(short param_index, std::vector<std::vector<uint8_t>> const& values, bool const* nulls);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void bind(
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        uint8_t const* null_sentry);

    /// @}

    /// \addtogroup bind_strings Binding multiple string values
    /// \brief Binds given string values to parameter marker in prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark) placeholders,
    /// this is how you bind values to them.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// It is possible to use these functions for batch operations.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param values Array of values to substitute into parameter placeholders.
    /// \param value_size Maximum length of string value in array.
    /// \param batch_size Number of string values to bind. Otherwise template parameter BatchSize is
    /// taken as the number of values. batch_size should greater or equal than row_count
    /// \param null_sentry Value which should represent a null value.
    /// \param nulls Flags for values that should be set to a null value.
    /// \throws database_error
    ///
    /// @{

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size);

    /// \brief Binds multiple string values.
    ///
    /// Size of the values vector indicates number of values to bind.
    /// Longest string in the array determines maximum length of individual value.
    ///
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(short param_index, std::vector<T> const& values);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void bind_strings(short param_index, T const (&values)[BatchSize][ValueSize])
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize);
    }

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        T const* null_sentry);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        short param_index,
        std::vector<T> const& values,
        typename T::value_type const* null_sentry);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void
    bind_strings(short param_index, T const (&values)[BatchSize][ValueSize], T const* null_sentry)
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize, nullptr, null_sentry);
    }

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        bool const* nulls);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(short param_index, std::vector<T> const& values, bool const* nulls);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void bind_strings(short param_index, T const (&values)[BatchSize][ValueSize], bool const* nulls)
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize, nulls);
    }

    void bind_null(short param_index);

    void describe_parameters(
        const std::vector<short>& idx,
        const std::vector<short>& type,
        const std::vector<unsigned long>& size,
        const std::vector<short>& scale);

    /// \brief Returns the number of columns in the table valued parameter.
    /// \throws database_error
    short parameters() const;

    /// \brief Returns parameter size for indicated column in the TVP.
    unsigned long parameter_size(short param_index) const;

    /// \brief Returns parameter scale for indicated column in the TVP.
    short parameter_scale(short param_index) const;

    /// \brief Returns parameter type for indicated column in the TVP.
    short parameter_type(short param_index) const;

private:
    class table_valued_parameter_impl;
    friend class statement;

private:
    std::shared_ptr<table_valued_parameter_impl> impl_;
};
#endif // NANODBC_DISABLE_MSSQL_TVP

// clang-format off
//  .d8888b.  888             888                                            888
// d88P  Y88b 888             888                                            888
// Y88b.      888             888                                            888
//  "Y888b.   888888  8888b.  888888 .d88b.  88888b.d88b.   .d88b.  88888b.  888888
//     "Y88b. 888        "88b 888   d8P  Y8b 888 "888 "88b d8P  Y8b 888 "88b 888
//       "888 888    .d888888 888   88888888 888  888  888 88888888 888  888 888
// Y88b  d88P Y88b.  888  888 Y88b. Y8b.     888  888  888 Y8b.     888  888 Y88b.
//  "Y8888P"   "Y888 "Y888888  "Y888 "Y8888  888  888  888  "Y8888  888  888  "Y888
// MARK: Statement -
// clang-format on

/// \brief Represents a statement on the database.
class statement
{

private:
    class statement_impl;

#ifdef NANODBC_HAS_STD_VARIANT
public:
    class attribute : public nanodbc::attribute
    {
    public:
        attribute(attribute const& other) noexcept
            : nanodbc::attribute(other){};
        attribute(
            long const& attribute,
            long const& string_length,
            variant const& resource) noexcept
            : nanodbc::attribute(attribute, string_length, resource){};

    private:
        friend class nanodbc::statement::statement_impl;
    };
#else
private:
    class attribute : public nanodbc::attribute
    {
    public:
        attribute(
            long const& attribute,
            long const& string_length,
            std::uintptr_t value_ptr) noexcept
            : nanodbc::attribute(attribute, string_length, value_ptr){};

    private:
        friend class nanodbc::statement::statement_impl;
    };
#endif

public:
    /// \brief Provides support for retrieving output/return parameters.
    /// \see binding
    enum param_direction
    {
        PARAM_IN,    ///< Binding an input parameter.
        PARAM_OUT,   ///< Binding an output parameter.
        PARAM_INOUT, ///< Binding an input/output parameter.
        PARAM_RETURN ///< Binding a return parameter.
    };

public:
    /// \brief Creates a new un-prepared statement.
    /// \see execute(), just_execute(), execute_direct(), just_execute_direct(), open(), prepare()
    statement();

    /// \brief Constructs a statement object and associates it to the given connection.
    /// \param conn The connection to use.
    /// \see open(), prepare()
    explicit statement(class connection& conn);

    explicit statement(class connection& conn, std::list<attribute> const& attributes);

    /// \brief Constructs and prepares a statement using the given connection and query.
    /// \param conn The connection to use.
    /// \param query The SQL query statement.
    /// \param timeout The number in seconds before query timeout. Default: 0 meaning no timeout.
    /// \see execute(), just_execute(), execute_direct(), just_execute_direct(), open(), prepare()
    statement(class connection& conn, string const& query, long timeout = 0);

    /// \brief Copy constructor.
    statement(const statement& rhs);

    /// \brief Move constructor.
    statement(statement&& rhs) noexcept;

    /// \brief Assignment.
    statement& operator=(statement rhs);

    /// \brief Member swap.
    void swap(statement& rhs) noexcept;

    /// \brief Closes the statement.
    /// \see close()
    ~statement() noexcept;

    /// \brief Creates a statement for the given connection.
    /// \param conn The connection where the statement will be executed.
    /// \throws database_error
    void open(class connection& conn);

    /// \brief Returns true if connection is open.
    bool open() const;

    /// \brief Returns true if connected to the database.
    bool connected() const;

    /// \brief Returns the associated connection object if any.
    class connection& connection();

    /// \brief Returns the associated connection object if any.
    const class connection& connection() const;

    /// \brief Returns the native ODBC statement handle.
    void* native_statement_handle() const;

    /// \brief Closes the statement and frees all associated resources.
    void close();

    /// \brief Cancels execution of the statement.
    /// \throws database_error
    void cancel();

    /// \brief Opens and prepares the given statement to execute on the given connection.
    /// \param conn The connection where the statement will be executed.
    /// \param query The SQL query that will be executed.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \see open()
    /// \throws database_error
    void prepare(class connection& conn, string const& query, long timeout = 0);

    /// \brief Prepares the given statement to execute its associated connection.
    /// \note If the statement is not open throws programming_error.
    /// \param query The SQL query that will be executed.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \see open()
    /// \throws database_error
    /// \throws programming_error
    void prepare(string const& query, long timeout = 0);

    /// \brief Sets the number in seconds before query timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    void timeout(long timeout = 0);

    /// \brief Opens, prepares, and executes the given query directly on the given connection.
    /// \param conn The connection where the statement will be executed.
    /// \param query The SQL query that will be executed.
    /// \param batch_operations Numbers of rows to fetch per rowset, or the number of batch
    ///        parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \return A result set object.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits occurring after each individual operation is executed.
    /// \see open(), prepare(), execute(), result, transaction
    class result execute_direct(
        class connection& conn,
        string const& query,
        long batch_operations = 1,
        long timeout = 0);

    /// \brief Opens, prepares, and executes the given query directly on the given connection.
    /// \param conn The connection where the statement will be executed.
    /// \param query The SQL query that will be executed.
    /// \param array_sizes More granular control of rows to fetch per rowset, and the number of
    ///                    batch parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \return A result set object.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits occurring after each individual operation is executed.
    /// \see open(), prepare(), execute(), result, transaction
    class result execute_direct(
        class connection& conn,
        string const& query,
        batch_ops const& array_sizes,
        long timeout = 0);

#if !defined(NANODBC_DISABLE_ASYNC)
    /// \brief Prepare the given statement, in asynchronous mode.
    /// \note If the statement is not open throws programming_error.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_STMT_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entirely by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \param event_handle The event handle the caller will wait before calling complete_prepare.
    /// \param query The SQL query that will be prepared.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \return Boolean: true if the event handle needs to be awaited, false is result is ready now.
    /// \see complete_prepare()
    bool async_prepare(string const& query, void* event_handle, long timeout = 0);

    /// \brief Completes a previously initiated asynchronous query preparation.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_STMT_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entirely by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \throws database_error
    /// \see async_prepare()
    void complete_prepare();

    /// \brief Opens, prepares, and executes query directly on the given connection, in async mode.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_STMT_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entirely by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \param conn The connection where the statement will be executed.
    /// \param event_handle The event handle the caller will wait before calling complete_execute.
    /// \param query The SQL query that will be executed.
    /// \param batch_operations Rows to fetch per rowset or number of batch parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \return Boolean: true if event handle needs to be awaited, false if result ready now.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits after each individual operation is executed.
    /// \see complete_execute(), open(), prepare(), execute(), result, transaction
    bool async_execute_direct(
        class connection& conn,
        void* event_handle,
        string const& query,
        long batch_operations = 1,
        long timeout = 0);

    /// \brief Execute the previously prepared query now, in asynchronous mode.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_STMT_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entirely by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \param event_handle The event handle the caller will wait before calling complete_execute.
    /// \param batch_operations Rows to fetch per rowset or number of batch parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \return Boolean: true if event handle needs to be awaited, false if result is ready now.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits after each individual operation is executed.
    /// \see complete_execute(), open(), prepare(), result, transaction
    bool async_execute(void* event_handle, long batch_operations = 1, long timeout = 0);

    /// \brief Completes a previously initiated asynchronous query execution, returning the result.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_STMT_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entirely by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \throws database_error
    /// \return A result set object.
    /// \param batch_operations Rows to fetch per rowset or number of batch parameters to process.
    /// \see async_execute(), async_execute_direct()
    class result complete_execute(long batch_operations = 1);

    /// undocumented - for internal use only (used from result_impl)
    void enable_async(void* event_handle);

    /// undocumented - for internal use only (used from result_impl)
    void disable_async() const;
#endif

    /// \brief Execute the previously prepared query now without constructing result object.
    /// \param conn The connection where the statement will be executed.
    /// \param query The SQL query that will be executed.
    /// \param batch_operations Rows to fetch per rowset, or number of batch parameters to process.
    /// \param timeout Seconds before query timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \return A result set object.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits after each individual operation is executed.
    /// \see open(), prepare(), execute(), execute_direct(), result, transaction
    void just_execute_direct(
        class connection& conn,
        string const& query,
        long batch_operations = 1,
        long timeout = 0);

    /// \brief Execute the previously prepared query now.
    /// \param batch_operations Rows to fetch per rowset, or number of batch parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \return A result set object.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits after each individual operation is executed.
    /// \see open(), prepare(), result, transaction
    class result execute(long batch_operations = 1, long timeout = 0);

    /// \brief Execute the previously prepared query now without constructing result object.
    /// \param batch_operations Rows to fetch per rowset, or number of batch parameters to process.
    /// \param timeout The number in seconds before query timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \return A result set object.
    /// \attention You will want to use transactions if you are doing batch operations because it
    ///            will prevent auto commits after each individual operation is executed.
    /// \see open(), prepare(), execute(), result, transaction
    void just_execute(long batch_operations = 1, long timeout = 0);

    /// \brief Returns the input and output paramters of the specified stored procedure.
    /// \param catalog The catalog name of the procedure.
    /// \param schema Pattern to use for schema names.
    /// \param procedure The name of the procedure.
    /// \param column Pattern to use for column names.
    /// \throws database_error
    /// \return A result set object.
    class result procedure_columns(
        string const& catalog,
        string const& schema,
        string const& procedure,
        string const& column);

    /// \brief Returns rows affected by the request or -1 if affected rows is not available.
    /// \throws database_error
    long affected_rows() const;

    /// \brief Returns the number of columns in a result set.
    /// \throws database_error
    short columns() const;

    /// \brief Resets all currently bound parameters.
    void reset_parameters() noexcept;

    /// \brief Returns the number of parameters in the statement.
    /// \throws database_error
    short parameters() const;

    /// \brief Returns parameter size for indicated parameter placeholder in a prepared statement.
    unsigned long parameter_size(short param_index) const;

    /// \brief Returns parameter scale for indicated parameter placeholder in a prepared statement.
    short parameter_scale(short param_index) const;

    /// \brief Returns parameter type for indicated parameter placeholder in a prepared statement.
    short parameter_type(short param_index) const;

    /// \addtogroup binding Binding parameters
    /// \brief These functions are used to bind values to ODBC parameters.
    ///
    /// @{

    /// \brief Binds given value to given parameter placeholder number in the prepared statement.
    ///
    /// If your prepared SQL query has any ? placeholders, this is how you bind values to them.
    /// Placeholder numbers count from left to right and are 0-indexed.
    ///
    /// It is NOT possible to use these functions for batch operations as number of elements is not
    /// specified here.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param value Value to substitute into placeholder.
    /// \param direction ODBC parameter direction.
    /// \throws database_error
    template <class T>
    void bind(short param_index, T const* value, param_direction direction = PARAM_IN);

    /// \addtogroup bind_multi Binding multiple non-string values
    /// \brief Binds given values to given parameter placeholder number in the prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark) placeholders,
    /// this is how you bind values to them.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// It is possible to use these functions for batch operations.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param values Values to substitute into placeholder.
    /// \param batch_size The number of values being bound.
    /// \param null_sentry Value which should represent a null value.
    /// \param nulls Flags for values that should be set to a null value.
    /// \param param_direction ODBC parameter direction.
    /// \throws database_error
    ///
    /// @{

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(
        short param_index,
        T const* values,
        std::size_t batch_size,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(
        short param_index,
        T const* values,
        std::size_t batch_size,
        T const* null_sentry,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple values.
    /// \see bind_multi
    template <class T>
    void bind(
        short param_index,
        T const* values,
        std::size_t batch_size,
        bool const* nulls,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void bind(
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void bind(
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        bool const* nulls,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple values.
    /// \see bind_multi
    void bind(
        short param_index,
        std::vector<std::vector<uint8_t>> const& values,
        uint8_t const* null_sentry,
        param_direction direction = PARAM_IN);

    /// @}

    /// \addtogroup bind_strings Binding multiple string values
    /// \brief Binds given string values to parameter marker in prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark) placeholders,
    /// this is how you bind values to them.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// It is possible to use these functions for batch operations.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param values Array of values to substitute into parameter placeholders.
    /// \param value_size Maximum length of string value in array.
    /// \param batch_size Number of string values to bind. Otherwise template parameter BatchSize is
    /// taken as the number of values.
    /// \param null_sentry Value which should represent a null value.
    /// \param nulls Flags for values that should be set to a null value.
    /// \param param_direction ODBC parameter direction.
    /// \throws database_error
    ///
    /// @{

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    ///
    /// Size of the values vector indicates number of values to bind.
    /// Longest string in the array determines maximum length of individual value.
    ///
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        short param_index,
        std::vector<T> const& values,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const (&values)[BatchSize][ValueSize],
        param_direction direction = PARAM_IN)
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize, direction);
    }

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        T const* null_sentry,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        short param_index,
        std::vector<T> const& values,
        typename T::value_type const* null_sentry,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const (&values)[BatchSize][ValueSize],
        T const* null_sentry,
        param_direction direction = PARAM_IN)
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize, null_sentry, direction);
    }

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const* values,
        std::size_t value_size,
        std::size_t batch_size,
        bool const* nulls,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <class T, typename = enable_if_string<T>>
    void bind_strings(
        short param_index,
        std::vector<T> const& values,
        bool const* nulls,
        param_direction direction = PARAM_IN);

    /// \brief Binds multiple string values.
    /// \see bind_strings
    template <
        std::size_t BatchSize,
        std::size_t ValueSize,
        class T,
        typename = enable_if_character<T>>
    void bind_strings(
        short param_index,
        T const (&values)[BatchSize][ValueSize],
        bool const* nulls,
        param_direction direction = PARAM_IN)
    {
        auto param_values = reinterpret_cast<T const*>(values);
        bind_strings(param_index, param_values, ValueSize, BatchSize, nulls, direction);
    }

    /// @}

    /// \brief Binds null values to the parameter placeholder number in the prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark) placeholders,
    /// this is how you bind values to them.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// It is possible to use this function for batch operations.
    ///
    /// \param param_index Zero-based index of parameter marker (placeholder position).
    /// \param batch_size The number of elements being bound.
    /// \throws database_error
    void bind_null(short param_index, std::size_t batch_size = 1);

    /// @}

    /// \brief Binds binary data.
    void bind_binary(
        short param_index,
        const uint8_t* value,
        std::size_t value_size,
        param_direction direction = PARAM_IN);

    /// \brief Binds binary data.
    void bind_binary(
        short param_index,
        std::vector<uint8_t> const& value,
        param_direction direction = PARAM_IN);

    /// \brief Binds binary data.
    void bind_binary(
        short param_index,
        std::vector<uint8_t> const& value,
        bool const* nulls,
        param_direction direction = PARAM_IN);

    /// \brief Binds binary data.
    void bind_binary(
        short param_index,
        std::vector<uint8_t> const& value,
        uint8_t const* null_sentry,
        param_direction direction = PARAM_IN);

    /// \brief Sets descriptions for parameters in the prepared statement.
    ///
    /// If your prepared SQL query has any parameter markers, ? (question  mark)
    /// placeholders this is how you can describe the SQL type, size and scale
    /// for some or all of the parameters, prior to binding any data to the
    /// parameters.  Calling this method is optional: if a parameter is not
    /// described using a call to this method, then during a bind an attempt is
    /// made to identify it using a call to the ODBC SQLDescribeParam API handle.
    /// Once set, description is re-used for possibly repeated binds
    /// execution and only cleared when the statement is cleared / destroyed.
    /// Parameter markers are numbered using Zero-based index from left to right.
    ///
    /// \param idx Vector of zero-based indices of parameters we are describing.
    /// \param type Vector of (short integer) types.
    /// \param size Vector of (unsigned long) sizes.
    /// \param scale Vector of (short integer) decimal precision / scale.
    /// \throws programming_error
    void describe_parameters(
        const std::vector<short>& idx,
        const std::vector<short>& type,
        const std::vector<unsigned long>& size,
        const std::vector<short>& scale);

    /// @}
private:
    typedef std::function<bool(std::size_t)> null_predicate_type;
    friend class nanodbc::result;
#ifndef NANODBC_DISABLE_MSSQL_TVP
    friend class nanodbc::table_valued_parameter::table_valued_parameter_impl;
#endif

private:
    std::shared_ptr<statement_impl> impl_;
};

// clang-format off
//  .d8888b.                                               888    d8b
// d88P  Y88b                                              888    Y8P
// 888    888                                              888
// 888         .d88b.  88888b.  88888b.   .d88b.   .d8888b 888888 888  .d88b.  88888b.
// 888        d88""88b 888 "88b 888 "88b d8P  Y8b d88P"    888    888 d88""88b 888 "88b
// 888    888 888  888 888  888 888  888 88888888 888      888    888 888  888 888  888
// Y88b  d88P Y88..88P 888  888 888  888 Y8b.     Y88b.    Y88b.  888 Y88..88P 888  888
//  "Y8888P"   "Y88P"  888  888 888  888  "Y8888   "Y8888P  "Y888 888  "Y88P"  888  888
// MARK: Connection -
// clang-format on

/// \brief Manages and encapsulates ODBC resources such as the connection and environment handles.
class connection
{

private:
    class connection_impl;
    friend class nanodbc::transaction::transaction_impl;

#ifdef NANODBC_HAS_STD_VARIANT
public:
    class attribute : public nanodbc::attribute
    {
    public:
        attribute(attribute const& other) noexcept
            : nanodbc::attribute(other){};
        attribute(
            long const& attribute,
            long const& string_length,
            variant const& resource) noexcept
            : nanodbc::attribute(attribute, string_length, resource){};

    private:
        friend class nanodbc::connection::connection_impl;
    };
#else
private:
    class attribute : public nanodbc::attribute
    {
    public:
        attribute(
            long const& attribute,
            long const& string_length,
            std::uintptr_t value_ptr) noexcept
            : nanodbc::attribute(attribute, string_length, value_ptr){};

    private:
        friend class nanodbc::connection::connection_impl;
    };
#endif
public:
    /// \brief Create new connection object, initially not connected.
    connection();

    /// Copy constructor.
    connection(const connection& rhs);

    /// Move constructor.
    connection(connection&& rhs) noexcept;

    /// Assignment.
    connection& operator=(connection rhs);

    /// Member swap.
    void swap(connection&) noexcept;

    /// \brief Create new connection object and immediately connect to the given data source.
    ///
    /// The function calls ODBC API SQLConnect.
    ///
    /// \param dsn The name of the data source name (DSN).
    /// \param user The username for authenticating to the data source.
    /// \param pass The password for authenticating to the data source.
    /// \param timeout Seconds before connection timeout. Default 0 meaning no timeout.
    /// \throws database_error
    /// \see connected(), connect()
    connection(string const& dsn, string const& user, string const& pass, long timeout = 0);

    /// \brief Create new connection object and immediately connect using the given connection
    /// string.
    ///
    /// The function calls ODBC API SQLDriverConnect.
    ///
    /// \param connection_string The connection string for establishing a connection.
    /// \param timeout Seconds before connection timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \see connected(), connect()
    explicit connection(string const& connection_string, long timeout = 0);

#ifdef NANODBC_HAS_STD_VARIANT
    /// \brief Create new connection object, set the connection attributes passed as
    /// arguments and connect to the given data source.
    ///
    /// The function calls ODBC API SQLConnect.  To set connection attributes,
    /// SQLSetConnectAttr is called.
    ///
    /// \param dsn The name of the data source name (DSN).
    /// \param user The username for authenticating to the data source.
    /// \param pass The password for authenticating to the data source.
    /// \param attributes A list of connection attributes to be set prior to connecting.
    /// \throws database_error
    /// \see connected(), connect(), attribute

    connection(
        string const& dsn,
        string const& user,
        string const& pass,
        std::list<attribute> const& attributes);
    /// \brief Create new connection object, set the connection attributes passed as
    /// arguments and connect to the given connection string.
    ///
    /// The function calls ODBC API SQLDriverConnect.  To set connection attributes,
    /// SQLSetConnectAttr is called.
    ///
    /// \param connection_string The connection string for establishing a connection.
    /// \param attributes A list of connection attributes to be set prior to connecting.
    /// \throws database_error
    /// \see connected(), connect(), attribute
    connection(string const& connection_string, std::list<attribute> const& attributes);
#endif
    /// \brief Automatically disconnects from the database and frees all associated resources.
    ///
    /// Will not throw even if disconnecting causes some kind of error and raises an exception.
    /// If you explicitly need to know if disconnect() succeeds, call it directly.
    ~connection() noexcept;

    /// \brief Allocate environment and connection handles.
    ///
    /// Allows on-demand allocation of handles to configure the ODBC environment
    /// and attributes, before database connection is established.
    /// Typically, user does not have to make this call explicitly.
    ///
    /// \throws database_error
    /// \see deallocate()
    void allocate();

    /// \brief Release environment and connection handles.
    /// \see allocate()
    void deallocate();

    /// \brief Connect to the given data source.
    /// \param dsn The name of the data source.
    /// \param user The username for authenticating to the data source.
    /// \param pass The password for authenticating to the data source.
    /// \param timeout Seconds before connection timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \see connected()
    void connect(string const& dsn, string const& user, string const& pass, long timeout = 0);

    /// \brief Connect using the given connection string.
    /// \param connection_string The connection string for establishing a connection.
    /// \param timeout Seconds before connection timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \see connected()
    void connect(string const& connection_string, long timeout = 0);

#ifdef NANODBC_HAS_STD_VARIANT
    /// \brief Set the connection attributes passed by the user, and connect to the given
    /// data source.
    /// \param dsn The name of the data source.
    /// \param user The username for authenticating to the data source.
    /// \param pass The password for authenticating to the data source.
    /// \param attributes A list of connection attributes to be set prior to connecting.
    /// \throws database_error
    /// \see connected(), attribute
    void connect(
        string const& dsn,
        string const& user,
        string const& pass,
        std::list<attribute> const& attributes);

    /// \brief Set the connection attributes passed by the user, and connect to the given
    /// connection string.
    /// \param connection_string The connection string for establishing a connection.
    /// \param attributes A list of connection attributes to be set prior to connecting.
    /// \throws database_error
    /// \see connected(), attribute
    void connect(string const& connection_string, std::list<attribute> const& attributes);
#endif
#if !defined(NANODBC_DISABLE_ASYNC)
    /// \brief Initiate an asynchronous connection operation to the given data source.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_DBC_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entierly by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \param dsn The name of the data source.
    /// \param user The username for authenticating to the data source.
    /// \param pass The password for authenticating to the data source.
    /// \param event_handle The event handle the caller will wait before calling async_complete.
    /// \param timeout Seconds before connection timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \return Boolean: true if event handle needs to be awaited, false if connection is ready now.
    /// \see connected()
    bool async_connect(
        string const& dsn,
        string const& user,
        string const& pass,
        void* event_handle,
        long timeout = 0);

    /// \brief Initiate an asynchronous connection operation using the given connection string.
    ///
    /// This method will only be available if nanodbc is built against ODBC headers and library that
    /// supports asynchronous mode. Such that the identifiers `SQL_ATTR_ASYNC_DBC_EVENT` and
    /// `SQLCompleteAsync` are extant. Otherwise this method will be defined, but not implemented.
    ///
    /// Asynchronous features can be disabled entierly by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    ///
    /// \param connection_string The connection string for establishing a connection.
    /// \param event_handle Event handle the caller will wait before calling async_complete.
    /// \param timeout Seconds before connection timeout. Default is 0 indicating no timeout.
    /// \throws database_error
    /// \return Boolean: true if event handle needs to be awaited, false if connection is ready now.
    /// \see connected()
    bool async_connect(string const& connection_string, void* event_handle, long timeout = 0);

    /// \brief Completes a previously initiated asynchronous connection operation.
    ///
    /// Asynchronous features can be disabled entierly by defining `NANODBC_DISABLE_ASYNC` when
    /// building nanodbc.
    void async_complete();
#endif

    /// \brief Returns true if connected to the database.
    bool connected() const;

    /// \brief Disconnects from the database, but maintains environment and handle resources.
    void disconnect();

    /// \brief Returns the number of transactions currently held for this connection.
    std::size_t transactions() const;

    /// \brief Returns the native ODBC database connection handle.
    void* native_dbc_handle() const;

    /// \brief Returns the native ODBC environment handle.
    void* native_env_handle() const;

    /// \brief Returns information from the ODBC connection as a string or fixed-size value.
    /// The general information about the driver and data source associated
    /// with a connection is obtained using `SQLGetInfo` function.
    template <class T>
    T get_info(short info_type) const;

    /// \brief Returns name of the DBMS product.
    /// Returns the ODBC information type SQL_DBMS_NAME of the DBMS product
    /// accesssed by the driver via the current connection.
    string dbms_name() const;

    /// \brief Returns version of the DBMS product.
    /// Returns the ODBC information type SQL_DBMS_VER of the DBMS product
    /// accesssed by the driver via the current connection.
    string dbms_version() const;

    /// \brief Returns the name of the ODBC driver.
    /// \throws database_error
    string driver_name() const;

    /// \brief Returns the version of the ODBC driver.
    /// \throws database_error
    string driver_version() const;

    /// \brief Returns the name of the currently connected database.
    /// Returns the current SQL_DATABASE_NAME information value associated with the connection.
    string database_name() const;

    /// \brief Returns the name of the current catalog.
    /// Returns the current setting of the connection attribute SQL_ATTR_CURRENT_CATALOG.
    string catalog_name() const;

private:
    std::size_t ref_transaction();
    std::size_t unref_transaction();
    bool rollback() const;
    void rollback(bool onoff);

private:
    std::shared_ptr<connection_impl> impl_;
};

// clang-format off
// 8888888b.                            888 888
// 888   Y88b                           888 888
// 888    888                           888 888
// 888   d88P .d88b.  .d8888b  888  888 888 888888
// 8888888P" d8P  Y8b 88K      888  888 888 888
// 888 T88b  88888888 "Y8888b. 888  888 888 888
// 888  T88b Y8b.          X88 Y88b 888 888 Y88b.
// 888   T88b "Y8888   88888P'  "Y88888 888  "Y888
// MARK: Result -
// clang-format on

class catalog;
class variant_row_cached_result;

/// \brief A resource for managing result sets from statement execution.
///
/// \see statement::execute(), statement::execute_direct()
/// \note result objects may be copied, however all copies will refer to the same result set.
class result
{
public:
    /// \brief Empty result set.
    result();

    /// \brief Free result set.
    ~result() noexcept;

    /// \brief Copy constructor.
    result(const result& rhs);

    /// \brief Move constructor.
    result(result&& rhs) noexcept;

    /// \brief Assignment.
    result& operator=(result rhs);

    /// \brief Member swap.
    void swap(result& rhs) noexcept;

    /// \brief Returns the native ODBC statement handle.
    void* native_statement_handle() const;

    /// \brief The rowset size for this result set.
    long rowset_size() const noexcept;

    /// \brief Number of affected rows by the request or -1 if the affected rows is not available.
    /// \throws database_error
    long affected_rows() const;

    /// \brief Reports if number of affected rows is available.
    /// \return true if number of affected rows is known, regardless of the value;
    /// false if the number is not available.
    /// \throws database_error {
    /// \code{.cpp}
    /// assert(r.has_affected_rows() == (r.affected_rows() >= 0));
    /// \endcode
    /// }
    bool has_affected_rows() const;

    /// \brief Rows in the current rowset or 0 if the number of rows is not available.
    long rows() const noexcept;

    /// \brief Returns the number of columns in a result set.
    /// \throws database_error
    short columns() const;

    /// \brief Fetches the first row in the current result set.
    /// \return true if there are more results or false otherwise.
    /// \throws database_error
    bool first();

    /// \brief Fetches the last row in the current result set.
    /// \return true if there are more results or false otherwise.
    /// \throws database_error
    bool last();

    /// \brief Fetches the next row in the current result set.
    /// \return true if there are more results or false otherwise.
    /// \throws database_error
    bool next();

#if !defined(NANODBC_DISABLE_ASYNC)
    /// \brief Initiates an asynchronous fetch of the next row in the current result set.
    /// \return true if the caller needs to wait for the event to be signalled, false if
    ///         complete_next() can be called immediately.
    /// \throws database_error
    bool async_next(void* event_handle);

    /// \brief Completes a previously-initiated async fetch for next row in the current result set.
    /// \return true if there are more results or false otherwise.
    /// \throws database_error
    bool complete_next();
#endif

    /// \brief Fetches the prior row in the current result set.
    /// \return true if there are more results or false otherwise.
    /// \throws database_error
    bool prior();

    /// \brief Moves to and fetches the specified row in the current result set.
    /// \return true if there are results or false otherwise.
    /// \throws database_error
    bool move(long row);

    /// \brief Skips a number of rows and then fetches the resulting row in the current result set.
    /// \return true if there are results or false otherwise.
    /// \throws database_error
    bool skip(long rows);

    /// \brief Returns the row position in the current result set.
    unsigned long position() const;

    /// \brief Returns true if there are no more results in the current result set.
    bool at_end() const noexcept;

    /// \brief Unbind data buffers for all columns in the result set.
    ///
    /// Wraps unbind(short column)
    /// \throws index_range_error
    /// \throws database_error
    void unbind();

    /// \brief Unbind data buffers for specific columns in the result set.
    ///
    /// Wraps unbind(short column)
    ///
    /// \param column_name string Name of column we wish to unbind.
    /// \throws index_range_error
    /// \throws database_error
    void unbind(string const& column_name);

    /// \brief Unbind data buffers for specific columns in the result set.
    ///
    /// When a result is constructed, in order to optimize data retrieval,
    /// we automatically try to bind buffers, except for columns that contain
    /// long/blob data types.  This method gives the caller the option to unbind
    /// a specific buffer.  Subsequently, during calls to get(), if there is no
    /// bound data buffer, we will attempt to retrieve the data using a call
    /// SQLGetData; this is similar to the route taken for columns hosting long
    /// or bloby data types.  This is suboptimal from efficiency perspective,
    /// however may be necessary of the driver we are communicating with does
    /// not support out-of-order retrieval of long data.
    ///
    /// \param column short Zero-based index of column we wish to unbind.
    /// \throws index_range_error
    /// \throws database_error
    void unbind(short column);

    /// \brief Gets data from the given column of the current rowset.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \param result The column's value will be written to this parameter.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    /// \throws null_access_error
    template <class T>
    void get_ref(short column, T& result) const;

    /// \brief Gets data from the given column of the current rowset.
    ///
    /// If the data is null, fallback is returned instead.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \param fallback if value is null, return fallback instead.
    /// \param result The column's value will be written to this parameter.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    template <class T>
    void get_ref(short column, T const& fallback, T& result) const;

    /// \brief Gets data from the given column by name of the current rowset.
    ///
    /// \param column_name column's name.
    /// \param result The column's value will be written to this parameter.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    /// \throws null_access_error
    template <class T>
    void get_ref(string const& column_name, T& result) const;

    /// \brief Gets data from the given column by name of the current rowset.
    ///
    /// If the data is null, fallback is returned instead.
    ///
    /// \param column_name column's name.
    /// \param fallback if value is null, return fallback instead.
    /// \param result The column's value will be written to this parameter.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    template <class T>
    void get_ref(string const& column_name, T const& fallback, T& result) const;

    /// \brief Gets data from the given column of the current rowset.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    /// \throws null_access_error
    template <class T>
    T get(short column) const;

    /// \brief Gets data from the given column of the current rowset.
    ///
    /// If the data is null, fallback is returned instead.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \param fallback if value is null, return fallback instead.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    template <class T>
    T get(short column, T const& fallback) const;

    /// \brief Gets data from the given column by name of the current rowset.
    ///
    /// \param column_name column's name.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    /// \throws null_access_error
    template <class T>
    T get(string const& column_name) const;

    /// \brief Gets data from the given column by name of the current rowset.
    ///
    /// If the data is null, fallback is returned instead.
    ///
    /// \param column_name column's name.
    /// \param fallback if value is null, return fallback instead.
    /// \throws database_error
    /// \throws index_range_error
    /// \throws type_incompatible_error
    template <class T>
    T get(string const& column_name, T const& fallback) const;

    /// \brief Returns true if and only if the given column of the current rowset is null.
    ///
    /// There is a bug/limitation in ODBC drivers for SQL Server (and possibly others)
    /// which causes SQLBindCol() to never write SQL_NOT_NULL to the length/indicator
    /// buffer unless you also bind the data column. nanodbc's is_null() will return
    /// correct values for (n)varchar(max) columns when you ensure that SQLGetData()
    /// has been called for that column (i.e. after get() or get_ref() is called).
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \see get(), get_ref()
    /// \param column position.
    /// \throws database_error
    /// \throws index_range_error
    bool is_null(short column) const;

    /// \brief Returns true if and only if the given column by name of the current rowset is null.
    ///
    /// See is_null(short column) for details on a bug/limitation of some ODBC drivers.
    /// \see is_null()
    /// \param column_name column's name.
    /// \throws database_error
    /// \throws index_range_error
    bool is_null(string const& column_name) const;

    /// \brief Returns true if we have bound a buffer to the given column.
    ///
    /// Generically, nanodbc will greedily bind buffers to columns in the result
    /// set.  However, we have also given the user the ability to unbind buffers
    /// via unbind() forcing nanodbc to retrieve data via SQLGetData.  This
    /// method returns true if there is a buffer bound to the column.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column short position.
    /// \throws index_range_error
    bool is_bound(short column) const;

    /// \brief Returns true if we have bound a buffer to the given column.
    ///
    /// See is_bound(short column) for details.
    /// \see is_bound()
    /// \param column_name column's name.
    /// \throws index_range_error
    bool is_bound(string const& column_name) const;

    /// \brief Returns the column number of the specified column name.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column_name column's name.
    /// \throws index_range_error
    short column(string const& column_name) const;

    /// \brief Returns the name of the specified column.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \throws index_range_error
    string column_name(short column) const;

    /// \brief Returns the size of the specified column.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \throws index_range_error
    long column_size(short column) const;

    /// \brief Returns the size of the specified column by name.
    long column_size(string const& column_name) const;

    /// \brief Returns the number of decimal digits of the specified column.
    ///
    /// Applies to exact numeric types (scale), datetime and interval types (prcision).
    /// If the number cannot be determined or is not applicable, drivers typically return 0.
    ///
    /// Columns are numbered from left to right and 0-indexed.
    /// \param column position.
    /// \throws index_range_error
    int column_decimal_digits(short column) const;

    /// \brief Returns the number of decimal digits of the specified column by name.
    int column_decimal_digits(string const& column_name) const;

    /// \brief Returns a identifying integer value representing the SQL type of this column.
    int column_datatype(short column) const;

    /// \brief Returns a identifying integer value representing the SQL type of this column by name.
    int column_datatype(string const& column_name) const;

    /// \brief Returns data source dependent data type name of this column.
    ///
    /// The function calls SQLCoLAttribute with the field attribute SQL_DESC_TYPE_NAME to
    /// obtain the data type name.
    /// If the type is unknown, an empty string is returned.
    /// \note Unlike other column metadata functions (eg. column_datatype()),
    /// this function cost is an extra ODBC API call.
    string column_datatype_name(short column) const;

    /// \brief Returns data source dependent data type name of this column by name.
    ///
    /// The function calls SQLCoLAttribute with the field attribute SQL_DESC_TYPE_NAME to
    /// obtain the data type name.
    /// If the type is unknown, an empty string is returned.
    /// \note Unlike other column metadata functions (eg. column_datatype()),
    /// this function cost is an extra ODBC API call.
    string column_datatype_name(string const& column_name) const;

    /// \brief Returns a identifying integer value representing the C type of this column.
    int column_c_datatype(short column) const;

    /// \brief Returns a identifying integer value representing the C type of this column by name.
    int column_c_datatype(string const& column_name) const;

    /// \brief Returns the next result, e.g. when stored procedure returns multiple result sets.
    bool next_result();

    /// \brief If and only if result object is valid, returns true.
    explicit operator bool() const;

private:
    result(statement statement, long rowset_size);

private:
    class result_impl;
    friend class nanodbc::statement::statement_impl;
    friend class nanodbc::catalog;
#ifdef _MSC_VER
    friend class nanodbc::variant_row_cached_result;
#endif

private:
    std::shared_ptr<result_impl> impl_;
};

/// \brief Single pass input iterator that accesses successive rows in the attached result set.
class result_iterator
{
public:
    typedef std::input_iterator_tag iterator_category; ///< Category of iterator.
    typedef result value_type;                         ///< Values returned by iterator access.
    typedef result* pointer;                           ///< Pointer to iteration values.
    typedef result& reference;                         ///< Reference to iteration values.
    typedef std::ptrdiff_t difference_type;            ///< Iterator difference.

    /// Default iterator; an empty result set.
    result_iterator() = default;

    /// Create result iterator for a given result set.
    explicit result_iterator(result& r)
        : result_(r)
    {
        ++(*this);
    }

    /// Dereference.
    reference operator*() { return result_; }

    /// Access through dereference.
    pointer operator->()
    {
        if (!result_)
            throw std::runtime_error("result is empty");
        return &(operator*());
    }

    /// Iteration.
    result_iterator& operator++()
    {
        try
        {
            if (!result_.next())
                result_ = result();
        }
        catch (...)
        {
            result_ = result();
        }
        return *this;
    }

    /// Iteration.
    result_iterator operator++(int)
    {
        result_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    /// Iterators are equal if they a tied to the same native statemnt handle, or both empty.
    bool operator==(result_iterator const& rhs) const
    {
        if (result_ && rhs.result_)
            return result_.native_statement_handle() == rhs.result_.native_statement_handle();
        else
            return !result_ && !rhs.result_;
    }

    /// Iterators are not equal if they have different native statemnt handles.
    bool operator!=(result_iterator const& rhs) const { return !(*this == rhs); }

private:
    result result_;
};

/// \brief Returns an iterator to the beginning of the given result set.
inline result_iterator begin(result& r)
{
    return result_iterator(r);
}

/// \brief Returns an iterator to the end of a result set.
///
/// The default-constructed `nanodbc::result_iterator` is known as the end-of-result iterator.
/// When a valid `nanodbc::result_iterator` reaches the end of the underlying result set,
/// it becomes equal to the end-of-result iterator.
/// Dereferencing or incrementing it further is undefined.
inline result_iterator end(result& /*r*/)
{
    return {};
}

// clang-format off
// 8888888b.                                     d8b          888
// 888  "Y88b                                    Y8P          888
// 888    888                                                 888
// 888    888  .d88b.  .d8888b   .d8888b 888d888 888 88888b.  888888 .d88b.  888d888 .d8888b
// 888    888 d8P  Y8b 88K      d88P"    888P"   888 888 "88b 888   d88""88b 888P"   88K
// 888    888 88888888 "Y8888b. 888      888     888 888  888 888   888  888 888     "Y8888b.
// 888  .d88P Y8b.          X88 Y88b.    888     888 888 d88P Y88b. Y88..88P 888          X88
// 8888888P"   "Y8888   88888P'  "Y8888P 888     888 88888P"   "Y888 "Y88P"  888      88888P'
//                                                   888
//                                                   888
//                                                   888
// MARK: Descriptors -
// clang-format on

/// Provides access to metadata in the Implementation Row Descriptor (IRD)
/// implicitly allocated for a prepared or executed statement.
///
/// The IRD contains information about the columns in a result set,
/// such as their SQL data types, lengths, and nullability.
class implementation_row_descriptor
{
public:
    /// Initializes IRD access from statement of executed result set.
    implementation_row_descriptor(result const& result);

    /// Initializes IRD access from prepared or executed statement.
    ///
    /// For performance reasons, an application should ensure the statement
    /// is executed before accessing any of the IRD fields.
    /// Accessing (i.e. calls to SQLGetDescRec) the descriptor fields of
    /// prepared only statement causes a roundtrip to SQL Server.
    ///
    /// \note Some fields of the descriptor are available on result set
    /// retrieved from statements that generate server cursors or
    /// on executed SQL Server `SELECT` statements containing a `FOR BROWSE`
    /// clause (database-specific).
    implementation_row_descriptor(statement const& statement);

    /// Value of the header field `SQL_DESC_ALLOC_AUTO`.
    auto alloc_type() const -> short;

    /// Value of the header field `SQL_DESC_COUNT`.
    auto count() const noexcept -> short;

    // Descriptor record fields (records) accessors

    /// Boolean based on value of the `SQL_DESC_AUTO_UNIQUE_VALUE` field.
    auto auto_unique_value(short record) const -> bool;

    /// Value of the `SQL_DESC_BASE_COLUMN_NAME` field.
    auto base_column_name(short record) const -> string;

    /// Value of the `SQL_DESC_BASE_TABLE_NAME` field.
    auto base_table_name(short record) const -> string;

    /// Boolean based on value of the `SQL_DESC_CASE_SENSITIVE` field.
    auto case_sensitive(short record) const -> bool;

    /// Value of the `SQL_DESC_CATALOG_NAME` field.
    auto catalog_name(short record) const -> string;

    /// Value of the `SQL_DESC_CONCISE_TYPE` field.
    auto concise_type(short record) const -> short;

    /// Value of the `SQL_DESC_DISPLAY_SIZE` field.
    auto display_size(short record) const -> std::int64_t;

    /// Value of the `SQL_DESC_FIXED_PREC_SCALE` field.
    auto fixed_prec_scale(short record) const -> short;

    /// Value of the `SQL_DESC_LABEL` field.
    auto label(short record) const -> string;

    /// Value of the `SQL_DESC_LENGTH` field.
    auto length(short record) const -> std::uint64_t;

    /// Value of the `SQL_DESC_LOCAL_TYPE_NAME` field.
    auto local_type_name(short record) const -> string;

    /// Value of the `SQL_DESC_NAME` field.
    auto name(short record) const -> string;

    /// Value of the `SQL_DESC_NULLABLE` field.
    ///
    /// \return Possible return values are `SQL_NULLABLE`, `SQL_NO_NULLS` or `SQL_NULLABLE_UNKNOWN`.
    auto nullable(short record) const -> short;

    /// Value of the `SQL_DESC_NUM_PREC_RADIX` field.
    auto num_prec_radix(short record) const -> short;

    /// Value of the `SQL_DESC_OCTET_LENGTH` field.
    auto octet_length(short record) const -> std::int64_t;

    /// Value of the `SQL_DESC_PRECISION` field.
    auto precision(short record) const -> short;

    /// Value of the `SQL_DESC_ROWVER` field.
    auto rowver(short record) const -> short;

    /// Value of the `SQL_DESC_SCALE` field.
    auto scale(short record) const -> short;

    /// Value of the `SQL_DESC_SCHEMA_NAME` field.
    auto schema_name(short record) const -> string;

    /// Value of the `SQL_DESC_SEARCHABLE` field.
    ///
    /// \return Possible return values are `SQL_PRED_NONE`, `SQL_PRED_CHAR`, `SQL_PRED_BASIC` or
    /// `SQL_PRED_SEARCHABLE`.
    auto searchable(short record) const -> short;

    /// Value of the `SQL_DESC_TABLE_NAME` field.
    auto table_name(short record) const -> string;

    /// Value of the `SQL_DESC_TYPE` field.
    auto type(short record) const -> short;

    /// Value of the `SQL_DESC_TYPE_NAME` field.
    auto type_name(short record) const -> string;

    /// Boolean based on value of the `SQL_DESC_UNNAMED` field.
    auto unnamed(short record) const -> bool;

    ///  Boolean based on value of the `SQL_DESC_UNSIGNED` field.
    auto unsigned_(short record) const -> bool;

    /// Value of the `SQL_DESC_UPDATABLE` field.
    ///
    /// \return Possible return values are `SQL_ATTR_READ_ONLY`, `SQL_ATTR_WRITE` or
    /// `SQL_ATTR_READWRITE_UNKNOWN`.
    auto updatable(short record) const -> short;

private:
    // Convenience wrapper for SQLGetDescrField accesor.
    struct sql_get_descr_field
    {
        sql_get_descr_field(
            implementation_row_descriptor const& ird,
            short record,
            std::uint16_t field_identifier) noexcept;
        operator std::int64_t() const;
        operator std::uint64_t() const;
        operator string() const;

        implementation_row_descriptor const& ird_;
        short record_;
        std::uint16_t field_identifier_;
    };
    friend sql_get_descr_field;

    void initialize_descriptor();
    void throw_if_record_is_out_of_range(short record) const;

    void* statement_handle_{nullptr};
    short statement_columns_count_{0};
    void* descriptor_handle_{nullptr};
    short descriptor_records_count_{0};
};

// clang-format off
//
//  .d8888b.           888             888
// d88P  Y88b          888             888
// 888    888          888             888
// 888         8888b.  888888  8888b.  888  .d88b.   .d88b.
// 888            "88b 888        "88b 888 d88""88b d88P"88b
// 888    888 .d888888 888    .d888888 888 888  888 888  888
// Y88b  d88P 888  888 Y88b.  888  888 888 Y88..88P Y88b 888
//  "Y8888P"  "Y888888  "Y888 "Y888888 888  "Y88P"   "Y88888
//                                                      888
//                                                 Y8b d88P
//                                                  "Y88P"
// MARK: Catalog -
// clang-format on

/// \brief A resource for get catalog information from connected data source.
///
/// Queries are performed using the Catalog Functions in ODBC.
/// All provided operations are convenient wrappers around the ODBC API
/// The original ODBC behaviour should not be affected by any added processing.
class catalog
{
public:
    /// \brief Result set for a list of tables in the data source.
    class tables
    {
    public:
        bool next();                  ///< Move to the next result in the result set.
        string table_catalog() const; ///< Fetch table catalog.
        string table_schema() const;  ///< Fetch table schema.
        string table_name() const;    ///< Fetch table name.
        string table_type() const;    ///< Fetch table type.
        string table_remarks() const; ///< Fetch table remarks.

    private:
        friend class nanodbc::catalog;
        explicit tables(result& find_result);
        result result_;
    };

    /// \brief Result set for a list of columns in one or more tables.
    class columns
    {
    public:
        bool next();                           ///< Move to the next result in the result set.
        string table_catalog() const;          ///< Fetch table catalog.
        string table_schema() const;           ///< Fetch table schema.
        string table_name() const;             ///< Fetch table name.
        string column_name() const;            ///< Fetch column name.
        short data_type() const;               ///< Fetch column data type.
        string type_name() const;              ///< Fetch column type name.
        long column_size() const;              ///< Fetch column size.
        long buffer_length() const;            ///< Fetch buffer length.
        short decimal_digits() const;          ///< Fetch decimal digits.
        short numeric_precision_radix() const; ///< Fetch numeric precission.
        short nullable() const;                ///< True iff column is nullable.
        string remarks() const;                ///< Fetch column remarks.
        string column_default() const;         ///< Fetch column's default.
        short sql_data_type() const;           ///< Fetch column's SQL data type.
        short sql_datetime_subtype() const;    ///< Fetch datetime subtype of column.
        long char_octet_length() const;        ///< Fetch char octet length.

        /// \brief Ordinal position of the column in the table.
        /// The first column in the table is number 1.
        /// Returns ORDINAL_POSITION column value in result set returned by SQLColumns.
        long ordinal_position() const;

        /// \brief Fetch column is-nullable information.
        ///
        /// \note MSDN: This column returns a zero-length string if nullability is unknown.
        ///       ISO rules are followed to determine nullability.
        ///       An ISO SQL-compliant DBMS cannot return an empty string.
        string is_nullable() const;

    private:
        friend class nanodbc::catalog;
        explicit columns(result& find_result);
        result result_;
    };

    /// \brief Result set for a list of columns that compose the primary key of a single table.
    class primary_keys
    {
    public:
        bool next();                  ///< Move to the next result in the result set.
        string table_catalog() const; ///< Fetch table catalog.
        string table_schema() const;  ///< Fetch table schema.
        string table_name() const;    ///< Fetch table name.
        string column_name() const;   ///< Fetch column name.

        /// \brief Column sequence number in the key (starting with 1).
        /// Returns valye of KEY_SEQ column in result set returned by SQLPrimaryKeys.
        short column_number() const;

        /// \brief Primary key name.
        /// NULL if not applicable to the data source.
        /// Returns valye of PK_NAME column in result set returned by SQLPrimaryKeys.
        string primary_key_name() const;

    private:
        friend class nanodbc::catalog;
        explicit primary_keys(result& find_result);
        result result_;
    };

    /// \brief Result set for a list of tables and the privileges associated with each table.
    class table_privileges
    {
    public:
        bool next();                  ///< Move to the next result in the result set
        string table_catalog() const; ///< Fetch table catalog.
        string table_schema() const;  ///< Fetch table schema.
        string table_name() const;    ///< Fetch table name.
        string grantor() const;       ///< Fetch name of user who granted the privilege.
        string grantee() const;       ///< Fetch name of user whom the privilege was granted.
        string privilege() const;     ///< Fetch the table privilege.
        /// Fetch indicator whether the grantee is permitted to grant the privilege to other users.
        string is_grantable() const;

    private:
        friend class nanodbc::catalog;
        explicit table_privileges(result& find_result);
        result result_;
    };

    /// \brief Result set for a list of procedures in the data source.
    class procedures
    {
    public:
        bool next();                      ///< Move to the next result in the result set.
        string procedure_catalog() const; ///< Fetch procedure catalog.
        string procedure_schema() const;  ///< Fetch procedure schema.
        string procedure_name() const;    ///< Fetch procedure name.
        string procedure_remarks() const; ///< Fetch procedure remarks.
        short procedure_type() const;     ///< Fetch procedure type.

    private:
        friend class nanodbc::catalog;
        explicit procedures(result& find_result);
        result result_;
    };

    /// \brief Result set for a list of procedures in the data source.
    class procedure_columns
    {
    public:
        bool next();                           ///< Move to the next result in the result set.
        string procedure_catalog() const;      ///< Fetch procedure catalog.
        string procedure_schema() const;       ///< Fetch procedure schema.
        string procedure_name() const;         ///< Fetch procedure name.
        string column_name() const;            ///< Fetch column name.
        short column_type() const;             ///< Fetch column type.
        short data_type() const;               ///< Fetch column data type.
        string type_name() const;              ///< Fetch column type name.
        long column_size() const;              ///< Fetch column size.
        long buffer_length() const;            ///< Fetch buffer length.
        short decimal_digits() const;          ///< Fetch decimal digits.
        short numeric_precision_radix() const; ///< Fetch numeric precission.
        short nullable() const;                ///< True iff column is nullable.
        string remarks() const;                ///< Fetch column remarks.
        string column_default() const;         ///< Fetch column's default.
        short sql_data_type() const;           ///< Fetch column's SQL data type.
        short sql_datetime_subtype() const;    ///< Fetch datetime subtype of column.
        long char_octet_length() const;        ///< Fetch char octet length.

        /// \brief Ordinal position of the column in the table.
        /// The first column in the table is number 1.
        /// Returns ORDINAL_POSITION column value in result set returned by SQLColumns.
        long ordinal_position() const;

        /// \brief Fetch column is-nullable information.
        ///
        /// \note MSDN: This column returns a zero-length string if nullability is unknown.
        ///       ISO rules are followed to determine nullability.
        ///       An ISO SQL-compliant DBMS cannot return an empty string.
        string is_nullable() const;

    private:
        friend class nanodbc::catalog;
        explicit procedure_columns(result& find_result);
        result result_;
    };

    /// \brief Creates catalog operating on database accessible through the specified connection.
    explicit catalog(connection& conn);

    /// \brief Creates result set with catalogs, schemas, tables, or table types.
    ///
    /// Tables information is obtained by executing `SQLTable` function within
    /// scope of the connected database accessible with the specified connection.
    /// Since this function is implemented in terms of the `SQLTable`s, it returns
    /// result set ordered by TABLE_TYPE, TABLE_CAT, TABLE_SCHEM, and TABLE_NAME.
    ///
    /// All arguments are treated as the Pattern Value Arguments.
    /// Empty string argument is equivalent to passing the search pattern '%'.
    catalog::tables find_tables(
        string const& table = string(),
        string const& type = string(),
        string const& schema = string(),
        string const& catalog = string());

    /// \brief Creates result set with tables and the privileges associated with each table.
    /// Tables information is obtained by executing `SQLTablePrivileges` function within
    /// scope of the connected database accessible with the specified connection.
    /// Since this function is implemented in terms of the `SQLTablePrivileges`s, it returns
    /// result set ordered by TABLE_CAT, TABLE_SCHEM, TABLE_NAME, PRIVILEGE, and GRANTEE.
    ///
    /// \param catalog The table catalog. It cannot contain a string search pattern.
    /// \param schema String search pattern for schema names, treated as the Pattern Value
    /// Arguments.
    /// \param table String search pattern for table names, treated as the Pattern Value Arguments.
    ///
    /// \note Due to the fact catalog cannot is not the Pattern Value Argument,
    ///       order of parameters is different than in the other catalog look-up functions.
    catalog::table_privileges find_table_privileges(
        string const& catalog,
        string const& table = string(),
        string const& schema = string());

    /// \brief Creates result set with columns in one or more tables.
    ///
    /// Columns information is obtained by executing `SQLColumns` function within
    /// scope of the connected database accessible with the specified connection.
    /// Since this function is implemented in terms of the `SQLColumns`, it returns
    /// result set ordered by TABLE_CAT, TABLE_SCHEM, TABLE_NAME, and ORDINAL_POSITION.
    ///
    /// All arguments are treated as the Pattern Value Arguments.
    /// Empty string argument is equivalent to passing the search pattern '%'.
    catalog::columns find_columns(
        string const& column = string(),
        string const& table = string(),
        string const& schema = string(),
        string const& catalog = string());

    /// \brief Creates result set with columns that compose the primary key of a single table.
    ///
    /// Returns result set with column names that make up the primary key for a table.
    /// The primary key information is obtained by executing `SQLPrimaryKey` function within
    /// scope of the connected database accessible with the specified connection.
    ///
    /// All arguments are treated as the Pattern Value Arguments.
    /// Empty string argument is equivalent to passing the search pattern '%'.
    catalog::primary_keys find_primary_keys(
        string const& table,
        string const& schema = string(),
        string const& catalog = string());

    /// \brief Creates result set with catalog, schema, procedure, and procedure types.
    ///
    /// Procedure information is obtained by executing `SQLProcedures` function within
    /// scope of the connected database accessible with the specified connection.
    /// Since this function is implemented in terms of the `SQLProcedures`s, it returns
    /// result set ordered by PROCEDURE_CAT, PROCEDUORE_SCHEM, and PROCEDURE_NAME.
    ///
    /// All arguments are treated as the Pattern Value Arguments.
    /// Empty string argument is equivalent to passing the search pattern '%'.

    catalog::procedures find_procedures(
        string const& procedure = string(),
        string const& schema = string(),
        string const& catalog = string());

    /// \brief Creates result set with columns in one or more procedures.
    ///
    /// Columns information is obtained by executing `SQLProcedureColumns` function within
    /// scope of the connected database accessible with the specified connection.
    /// Since this function is implemented in terms of the `SQLProcedureColumns`, it returns
    /// result set ordered by PROCEDURE_CAT, PROCEDURE_SCHEM, PROCEDURE_NAME, and
    /// COLUMN_TYPE.
    ///
    /// All arguments are treated as the Pattern Value Arguments.
    /// Empty string argument is equivalent to passing the search pattern '%'.
    catalog::procedure_columns find_procedure_columns(
        string const& column = string(),
        string const& procedure = string(),
        string const& schema = string(),
        string const& catalog = string());

    /// \brief Returns names of all catalogs (or databases) available in connected data source.
    ///
    /// Executes `SQLTables` function with `SQL_ALL_CATALOG` as catalog search pattern.
    std::list<string> list_catalogs();

    /// \brief Returns names of all schemas available in connected data source.
    ///
    /// Executes `SQLTables` function with `SQL_ALL_SCHEMAS` as schema search pattern.
    std::list<string> list_schemas();

    /// \brief Returns all available table types in the connected data source.
    ///
    /// Executes `SQLTables` function with `SQL_ALL_TABLE_TYPES` as the
    /// table type search pattern.
    std::list<string> list_table_types();

private:
    connection conn_;
};

/// @}

// clang-format off
// 8888888888                            8888888888                         888    d8b
// 888                                   888                                888    Y8P
// 888                                   888                                888
// 8888888 888d888 .d88b.   .d88b.       8888888 888  888 88888b.   .d8888b 888888 888  .d88b.  88888b.  .d8888b
// 888     888P"  d8P  Y8b d8P  Y8b      888     888  888 888 "88b d88P"    888    888 d88""88b 888 "88b 88K
// 888     888    88888888 88888888      888     888  888 888  888 888      888    888 888  888 888  888 "Y8888b.
// 888     888    Y8b.     Y8b.          888     Y88b 888 888  888 Y88b.    Y88b.  888 Y88..88P 888  888      X88
// 888     888     "Y8888   "Y8888       888      "Y88888 888  888  "Y8888P  "Y888 888  "Y88P"  888  888  88888P'
// MARK: Free Functions -
// clang-format on

/// \addtogroup mainf Free Functions
/// \brief Convenience functions.
///
/// @{

/// \brief Information on a configured ODBC driver.
struct driver
{
    /// \brief Driver attributes.
    struct attribute
    {
        nanodbc::string keyword; ///< Driver keyword attribute.
        nanodbc::string value;   ///< Driver attribute value.
    };

    nanodbc::string name;            ///< Driver name.
    std::list<attribute> attributes; ///< List of driver attributes.
};

struct datasource
{
    nanodbc::string name;   ///< DSN name.
    nanodbc::string driver; ///< Driver description.
};

/// \brief Returns a list of ODBC drivers on your system.
std::list<driver> list_drivers();

/// \brief Returns a list of ODBC data sources on your system.
std::list<datasource> list_datasources();

/// \brief Immediately opens, prepares, and executes the given query directly on the given
/// connection.
/// \param conn The connection where the statement will be executed.
/// \param query The SQL query that will be executed.
/// \param batch_operations Numbers of rows to fetch per rowset.
/// \param timeout The number in seconds before query timeout. Default is 0 indicating no timeout.
/// \return A result set object.
/// \attention You will want to use transactions if you are doing batch operations because it will
///            prevent auto commits from occurring after each individual operation is executed.
/// \see open(), prepare(), execute(), result, transaction
result execute(connection& conn, string const& query, long batch_operations = 1, long timeout = 0);

/// \brief Opens, prepares, and executes query directly without creating result object.
/// \param conn The connection where the statement will be executed.
/// \param query The SQL query that will be executed.
/// \param batch_operations Rows to fetch per rowset, or number of batch parameters to process.
/// \param timeout The number in seconds before query timeout. Default is 0 indicating no timeout.
/// \return A result set object.
/// \attention You will want to use transactions if you are doing batch operations because it will
///            prevent auto commits from occurring after each individual operation is executed.
/// \see open(), prepare(), execute(), result, transaction
void just_execute(
    connection& conn,
    string const& query,
    long batch_operations = 1,
    long timeout = 0);

/// \brief Execute the previously prepared query now.
/// \param stmt The prepared statement that will be executed.
/// \param batch_operations Rows to fetch per rowset, or the number of batch parameters to process.
/// \throws database_error
/// \return A result set object.
/// \attention You will want to use transactions if you are doing batch operations because it will
///            prevent auto commits from occurring after each individual operation is executed.
/// \see open(), prepare(), execute(), result
result execute(statement& stmt, long batch_operations = 1);

/// \brief Execute the previously prepared query now and without creating result object.
/// \param stmt The prepared statement that will be executed.
/// \param batch_operations Rows to fetch per rowset, or the number of batch parameters to process.
/// \throws database_error
/// \return A result set object.
/// \attention You will want to use transactions if you are doing batch operations because it will
///            prevent auto commits from occurring after each individual operation is executed.
/// \see open(), prepare(), execute(), result
void just_execute(statement& stmt, long batch_operations = 1);

/// \brief Execute the previously prepared query now.
///
/// Executes within the context of a transaction object, commits directly after execution.
/// \param stmt The prepared statement that will be executed in batch.
/// \param batch_operations Rows to fetch per rowset, or the number of batch parameters to process.
/// \throws database_error
/// \return A result set object.
/// \see open(), prepare(), execute(), result, transaction
result transact(statement& stmt, long batch_operations);

/// \brief Execute the previously prepared query now and without creating result object.
///
/// Executes within the context of a transaction object, commits directly after execution.
/// \param stmt The prepared statement that will be executed in batch.
/// \param batch_operations Rows to fetch per rowset, or the number of batch parameters to process.
/// \throws database_error
/// \return A result set object.
/// \see open(), prepare(), execute(), result, transaction
void just_transact(statement& stmt, long batch_operations);

/// \brief Prepares the given statement to execute on it associated connection.
///
/// If the statement is not open throws programming_error.
/// \param stmt The prepared statement that will be executed in batch.
/// \param query The SQL query that will be executed.
/// \param timeout The number in seconds before query timeout. Default is 0 indicating no timeout.
/// \see open()
/// \throws database_error
/// \throws programming_error
void prepare(statement& stmt, string const& query, long timeout = 0);

/// @}

} // namespace nanodbc

#endif
