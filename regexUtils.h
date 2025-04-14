#pragma once 


static inline bool validateExpression(std::string text, std::string regexPattern) {
    try {
        // Create a regex object from the provided pattern
        std::regex pattern(regexPattern);
        
        // Use regex_match to check if the entire text matches the pattern
        return std::regex_match(text, pattern);
    }
    catch (const std::regex_error& e) {
        // Handle invalid regex pattern
        std::cerr << "Regex error: " << e.what() << std::endl;
        return false;
    }
}