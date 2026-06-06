#ifndef UTILS_HPP
#define UTILS_HPP

#include "types.hpp"
#include <QStringList>
#include <QString>
#include <QMap>

class QMimeData;


namespace Utils
{

    bool hasValidFiles(const QMimeData* mimeData);

    QStringList toFilelist(const QMimeData* mimeData);

    template<std::floating_point T>
    T roundToHalf(T input)
    {
        return std::round(input * T(2)) / T(2);
    }

    /// \brief Create a predicate to be used with std::find_if etc. to match a property and its value
    /// \param propertyName The name of the property to be matched
    /// \param expectedValue The value of the property to be matched
    template<typename T>
    auto getPropertyEqualsPred(const char* propertyName, T expectedValue)
    {
        return [propertyName, expectedValue](const auto* object)
        {
            const auto variant = object->property(propertyName);
            if constexpr (std::is_enum_v<T>) /// is_enum_v doesn't differentiate between plain enum and enum class
            {
                /// Check enum class first (get_if<T> doesn't return plain enum)
                if(const T* prop_value = get_if<T>(&variant))
                {
                    return *prop_value == expectedValue;
                }
                /// Check plain enum using int
                if(const int* prop_value = get_if<int>(&variant))
                {
                    return static_cast<T>(*prop_value) == expectedValue;
                }
                return false;
            }
            else /// This branch is not really tested!
            {
                const T* prop_value = get_if<T>(&variant);
                return prop_value && (*prop_value == expectedValue);
            }
        };
    }


    static inline QString nameForEndOfLine(EndOfLine eol)
    {
        static const QMap<EndOfLine, QString> eolNames
            {
             { EndOfLine::UNAVAILABLE,   "N/A" },
             { EndOfLine::UNIX,          "Unix (LF)" },
             { EndOfLine::WINDOWS,       "Windows (CRLF)" },
             { EndOfLine::MAC,           "Mac legacy (CR)" },
             };
        return eolNames.value(eol);
    }
};


#endif // UTILS_HPP
