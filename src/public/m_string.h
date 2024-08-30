#ifndef M_STRING_H_
#define M_STRING_H_

#include <iostream>
#include <string>

namespace cyyu {

class m_string
{
public:
    /**
     * @brief Safely convert a string to an integer.
     *
     * This function attempts to convert a string to an integer.
     * If the conversion fails due to invalid input or range issues,
     * it returns false and outputs an error message.
     *
     * @param str The string to convert.
     * @param num Reference to an integer where the result will be stored.
     * @return True if the conversion was successful, false otherwise.
     */
    inline static bool stoi_safe(const std::string str, int& num) {
        if (!m_stoi(str, num)) {
            return false;
        }
        if (std::to_string(num) != str) {
            std::cerr << "Error: Mismatch between input string and converted integer." << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Convert a string to lowercase.
     *
     * This function converts all uppercase characters in a string to lowercase.
     *
     * @param str The string to convert.
     * @return The converted string.
     */
    inline static std::string to_lower(const std::string& str) {
        std::string result = "";
        for (auto& ch : result) {
            if ((ch >= 'A') && (ch <= 'Z')) ch = ch + 32;
        }
        return result;
    }

    /**
     * @brief Convert a string to uppercase.
     *
     * This function converts all lowercase characters in a string to uppercase.
     *
     * @param str The string to convert.
     * @return The converted string.
     */
    inline static std::string to_upper(const std::string& str) {
        std::string result = "";
        for (auto& ch : result) {
            if ((ch >= 'a') && (ch <= 'z')) ch = ch - 32;
        }
        return result;
    }

private:
    inline static bool m_stoi(const std::string str, int& num) {
        try {
            num = std::stoi(str);
            return true;
        }
        catch (const std::invalid_argument&) {
            std::cerr << "Error: Invalid input. The string is not a valid integer." << std::endl;
            return false;
        }
        catch (const std::out_of_range&) {
            std::cerr << "Error: The number is out of range for an int." << std::endl;
            return false;
        }
    }
};

}   // namespace cyyu

#endif   // M_STRING_H_