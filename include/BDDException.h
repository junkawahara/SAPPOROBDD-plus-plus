/*****************************************
 *  BDD Package (SAPPORO-1.94) Exception Classes  *
 *  (C) Shin-ichi MINATO                          *
 ******************************************/

#ifndef BDD_EXCEPTION_H
#define BDD_EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>
#include "bddc.h"

namespace SAPPOROBDD {

/**
 * Base exception class for all BDD errors
 */
class BDDException : public std::exception {
protected:
    std::string message;

public:
    BDDException(const std::string& msg) : message(msg) {}
    virtual ~BDDException() noexcept = default;

    virtual const char* what() const noexcept override {
        return message.c_str();
    }
    
    virtual std::string info() const {
        return message;
    }
};

/**
 * Exception thrown when an invalid bddp value is provided
 */
class BDDInvalidBDDValueException : public BDDException {
private:
    bddp value;

public:
    BDDInvalidBDDValueException(const std::string& msg, bddp val) 
        : BDDException(msg), value(val) {
        std::stringstream ss;
        ss << message << " (bddp value: 0x" << std::hex << value << ")";
        message = ss.str();
    }

    bddp getValue() const { return value; }
    
    std::string info() const override {
        return message;
    }
};

/**
 * Exception thrown when a value is outside of valid range
 */
class BDDOutOfRangeException : public BDDException {
private:
    bddp value;

public:
    BDDOutOfRangeException(const std::string& msg, bddp val) 
        : BDDException(msg), value(val) {
        std::stringstream ss;
        ss << message << " (value: " << value << ")";
        message = ss.str();
    }

    bddp getValue() const { return value; }
    
    std::string info() const override {
        return message;
    }
};

/**
 * Exception thrown when memory allocation fails
 */
class BDDOutOfMemoryException : public BDDException {
private:
    bddp requested_size;

public:
    BDDOutOfMemoryException(const std::string& msg, bddp size = 0) 
        : BDDException(msg), requested_size(size) {
        if (size > 0) {
            std::stringstream ss;
            ss << message << " (requested size: " << size << ")";
            message = ss.str();
        }
    }

    bddp getRequestedSize() const { return requested_size; }
    
    std::string info() const override {
        return message;
    }
};

/**
 * Exception thrown for file format errors during import/export
 */
class BDDFileFormatException : public BDDException {
private:
    bddp value;

public:
    BDDFileFormatException(const std::string& msg, bddp val = 0) 
        : BDDException(msg), value(val) {
        if (val != 0) {
            std::stringstream ss;
            ss << message << " (bddp value: 0x" << std::hex << value << ")";
            message = ss.str();
        }
    }

    bddp getValue() const { return value; }
    
    std::string info() const override {
        return message;
    }
};

/**
 * Exception thrown for internal package errors
 */
class BDDInternalErrorException : public BDDException {
private:
    bddp value;

public:
    BDDInternalErrorException(const std::string& msg, bddp val = 0) 
        : BDDException(msg), value(val) {
        if (val != 0) {
            std::stringstream ss;
            ss << message << " (value: 0x" << std::hex << val << ")";
            message = ss.str();
        }
    }
    
    bddp getValue() const { return value; }
    
    std::string info() const override {
        return message;
    }
};

} // namespace SAPPOROBDD

#endif // BDD_EXCEPTION_H
