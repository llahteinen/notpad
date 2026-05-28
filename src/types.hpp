#ifndef TYPES_HPP
#define TYPES_HPP


enum class EndOfLine
{
    UNAVAILABLE = -1,
    UNIX = 0,       /// lf
    WINDOWS = 1,    /// crlf
    MAC = 2,        /// cr (obsolete)
};


#endif // TYPES_HPP
