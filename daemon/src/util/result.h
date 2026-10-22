#pragma once

#include <QString>
#include <QtLogging>

#include <optional>
#include <utility>

namespace paraphrase::util {

/// A value or an error message for the user (DOCS/CODING_STANDARDS.md 6): what went wrong and
/// what to do, never a bool plus out-parameter.
template <class T>
class Result
{
public:
    static Result success(T value) { return Result(std::move(value), {}); }
    static Result failure(QString error) { return Result(std::nullopt, std::move(error)); }

    [[nodiscard]] bool ok() const { return m_value.has_value(); }
    /// The value; reading it from a failure is a programming error and stops the process.
    [[nodiscard]] const T& value() const&
    {
        if (!m_value.has_value()) {
            qFatal("Result::value() on a failure: %s", qPrintable(m_error));
        }
        return *m_value;
    }
    [[nodiscard]] T&& value() &&
    {
        if (!m_value.has_value()) {
            qFatal("Result::value() on a failure: %s", qPrintable(m_error));
        }
        return std::move(*m_value);
    }
    [[nodiscard]] const QString& error() const { return m_error; }

private:
    Result(std::optional<T> value, QString error)
        : m_value(std::move(value))
        , m_error(std::move(error))
    {}

    std::optional<T> m_value;
    QString m_error;
};

} // namespace paraphrase::util
