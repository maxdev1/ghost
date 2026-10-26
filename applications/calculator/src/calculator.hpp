/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __CALCULATOR__
#define __CALCULATOR__

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

class Calculator
{
    std::string entry = "0";
    std::string expression;
    double accumulator = 0;
    double repeatValue = 0;
    char operation = 0;
    char repeatOperation = 0;
    bool newEntry = true;
    bool resultShown = false;
    bool failed = false;

    static std::string format(double value)
    {
        if(value == 0) return "0";
        // Ghost's printf currently treats %g/%e as %f. Build a bounded,
        // 12-significant-digit representation without its float formatter.
        bool negative = value < 0;
        double magnitude = negative ? -value : value;
        int exponent = 0;
        while(magnitude >= 10) { magnitude /= 10; ++exponent; }
        while(magnitude < 1) { magnitude *= 10; --exponent; }
        auto digits = std::to_string(static_cast<uint64_t>(magnitude * 100000000000.0 + .5));
        if(digits.size() > 12) { digits.pop_back(); ++exponent; }
        while(digits.size() > 1 && digits.back() == '0') digits.pop_back();
        std::string result;
        if(exponent >= -4 && exponent < 12) {
            int point = exponent + 1;
            if(point <= 0) result = "0." + std::string(-point, '0') + digits;
            else if(point >= int(digits.size())) result = digits + std::string(point - digits.size(), '0');
            else result = digits.substr(0, point) + "." + digits.substr(point);
        }
        else {
            result = digits.substr(0, 1);
            if(digits.size() > 1) result += "." + digits.substr(1);
            result += std::string("e") + (exponent < 0 ? "-" : "+") + std::to_string(exponent < 0 ? -exponent : exponent);
        }
        return negative ? "-" + result : result;
    }
    double value() const { return std::strtod(entry.c_str(), nullptr); }
    bool apply(char op, double right)
    {
        if(op == '/' && right == 0) return error("Cannot divide by zero");
        if(op == '+') accumulator += right;
        else if(op == '-') accumulator -= right;
        else if(op == '*') accumulator *= right;
        else if(op == '/') accumulator /= right;
        if(!std::isfinite(accumulator)) return error("Result out of range");
        entry = format(accumulator);
        return true;
    }
    bool error(const std::string& message)
    {
        entry = "Error"; expression = message; failed = true;
        operation = repeatOperation = 0;
        return false;
    }
    void beginEntry()
    {
        if(newEntry) { entry = "0"; newEntry = false; }
        if(resultShown) { expression.clear(); repeatOperation = 0; resultShown = false; }
    }

public:
    const std::string& display() const { return entry; }
    const std::string& history() const { return expression; }
    char pendingOperation() const { return operation; }

    // Digits, decimal point, arithmetic, equals; c = clear, b = erase, s = sign.
    void input(char key)
    {
        if(key == 'c') { *this = Calculator(); return; }
        if(failed) {
            if((key >= '0' && key <= '9') || key == '.' || key == 'b') *this = Calculator();
            else return;
        }
        if(key >= '0' && key <= '9') {
            beginEntry();
            size_t digits = 0;
            for(char c : entry) if(c >= '0' && c <= '9') ++digits;
            if(digits >= 14) return;
            if(entry == "0") entry = key;
            else if(entry == "-0") entry = std::string("-") + key;
            else entry += key;
        }
        else if(key == '.') {
            beginEntry();
            if(entry.find('.') == std::string::npos) entry += '.';
        }
        else if(key == 'b') {
            if(newEntry) return;
            entry.pop_back();
            if(entry.empty() || entry == "-") entry = "0";
        }
        else if(key == 's') {
            if(newEntry && operation) entry = "0";
            if(resultShown) { expression.clear(); repeatOperation = 0; }
            else newEntry = false;
            if(entry[0] == '-') entry.erase(0, 1);
            else entry.insert(0, "-");
        }
        else if(key == '+' || key == '-' || key == '*' || key == '/') {
            if(operation && !newEntry) { if(!apply(operation, value())) return; }
            else if(!operation) accumulator = value();
            operation = key; repeatOperation = 0; newEntry = true; resultShown = false;
            expression = format(accumulator) + " " + key;
        }
        else if(key == '=') {
            char op = operation ? operation : repeatOperation;
            if(!op) return;
            double right = operation ? (newEntry ? accumulator : value()) : repeatValue;
            expression = format(accumulator) + " " + op + " " + format(right) + " =";
            if(!apply(op, right)) return;
            repeatOperation = op; repeatValue = right; operation = 0;
            newEntry = resultShown = true;
        }
    }
};

#endif
