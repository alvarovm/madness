#ifndef MADEXCEPT_H_
#define MADEXCEPT_H_

/// \file madexcept.h

namespace madness {
		
    class MadnessException {
    public:
        const char* msg;
        const char* assertion;
        const int value;
        const int line;
        const char *function;
        const char *filename;

        // Capturing the line/function/filename info is best done with the macros below
        MadnessException(const char* msg, const char *assertion, int value, 
                        int line, const char *function, const char *file)
                : msg(msg)
                , assertion(assertion)
                , value(value)
                , line(line)
                , function(function)
        		, filename(file) {};
    };

	// implemented in madexcept.cc
    std::ostream& operator <<(std::ostream& out, const MadnessException& e);

#define MADNESS_EXCEPTION(msg,value) \
throw MadnessException(msg,0,value,__LINE__,__FUNCTION__,__FILE__)

/*
 * Default behaviour is MADNESS_ASSERTION maps to C assert()
 * 
 * define DISABLE_MADNESS_ASSERTIONS to disable all MADNESS_ASSERTIONS at compile time.
 * define MADNESS_ASSERTIONS_THROW to make MADNESS_ASSERTIONS throw a MadnessException
 * 
 */

#ifdef DISABLE_MADNESS_ASSERTIONS
#  define MADNESS_ASSERT(condition)
#elif defined(MADNESS_ASSERTIONS_THROW)
#  define MADNESS_ASSERT(condition) \
     do {if (!(condition)) \
         throw MadnessException("MADNESS ASSERTION FAILED", \
                         #condition,0,__LINE__,__FUNCTION__,__FILE__); \
        } while (0)
#else
#  include <cassert>
#  define MADNESS_ASSERT(condition) assert(condition)
#endif

}
	
#endif /*MADEXCEPT_H_*/
