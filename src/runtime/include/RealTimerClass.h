/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#ifndef COUTIL_GLOBAL_REALTIMER
#define COUTIL_GLOBAL_REALTIMER

#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <sstream>
#ifdef DEBUG
static const char* profile_mode=getenv("ATMI_PROFILE");
#define USE_PROFILE
#endif

namespace Global {
  
/// \class RealTimer RealTimer.h "Global/RealTimer.h"
///
/// \brief Compute elapsed time.
class RealTimer
{
public:
  /// Constructor.  If desc is provides, it will be output along with
  /// the RealTimer state when the RealTimer object is destroyed.
  RealTimer(const std::string& desc="");
  ~RealTimer();

  /// Start the timer.
  void Start();
  
  /// Stop the timer.
  void Stop();

  /// Reset all counters to 0.
  void Reset();
  
  /// Return the total time during which the timer was running.
  double Elapsed() const;

  /// Number of times Stop() is called.
  int Count() const;
  
  void Pause();
  void Resume();

  /// Print the current state of the counter to the stream.
  void Print(std::ostream& o) const;

  /// Print the current state of the counter to the stream with custom description
  /// string
  void BufPrint(std::ostream& o, std::string& str) const;
  
  /// Return the system time.
  double CurrentTime() const;

  /// Return true if the timer is running (started), otherwise return false (stopped).
  bool IsRunning();

private:
  std::string fDesc;
  int time_offset;
  double start_time;
  double elapsed;
  bool isRunning;
  int count;
};

/// Timer stream insertion operator
inline std::ostream& operator<<(std::ostream& os, const RealTimer& t)
{
  t.Print(os);
  return os;
}

inline RealTimer::RealTimer(const std::string& desc)
  : fDesc(desc),
    time_offset(0),
    start_time(0),
    elapsed(0.0),
    isRunning(false),
    count(0)
{
  time_offset=(int)CurrentTime();
}

inline RealTimer::~RealTimer()
{
#if 0
  if (fDesc != "")
    std::cout << "Timer " << fDesc << std::endl; 
#endif
}

inline void RealTimer::Start()
{
#ifdef USE_PROFILE
    if(profile_mode) {
        static std::string functionName("RealTimer::Start()");
        if (isRunning == true) {
            std::cout << functionName << ": Warning: Timer " << fDesc << " has already been started." <<  std::endl; 
        } else {
            start_time = CurrentTime();
            isRunning = true;
        }
    }
#endif
}

inline void RealTimer::Stop()
{
#ifdef USE_PROFILE
    if(profile_mode) {
        static std::string functionName("RealTimer::Stop()");
        if (isRunning == false) {
            std::cout << functionName << ": Warning: Timer " << fDesc << " has already been stopped." <<  std::endl; 
        } else {
            elapsed += CurrentTime() - start_time;
            isRunning = false;
            count++;
        }
    }
#endif
}

inline bool
RealTimer::IsRunning()
{
  return isRunning;
}

inline void RealTimer::Reset()
{
  elapsed = 0.0;
  start_time = 0;
  count = 0;
  time_offset = 0;
  time_offset = (int)CurrentTime();
}

inline double RealTimer::Elapsed() const
{
  static std::string functionName("inline double Timer::Elapsed() const");
  if (isRunning == true) {
    std::cout << functionName << ": Warning: Timer " << fDesc << " is still running." <<  std::endl; 
    return elapsed + CurrentTime() - start_time;
  }
  return elapsed;
}

inline int RealTimer::Count() const {return count;}

inline double RealTimer::CurrentTime() const
{
#if 1
   timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   return (double)(ts.tv_sec-time_offset) + (double)ts.tv_nsec*1e-9;
#else
  timeval tv;
  gettimeofday(&tv, NULL);
  return (double)(tv.tv_sec-time_offset) + (double)tv.tv_usec*1e-6;
#endif
}


inline void RealTimer::Print(std::ostream& o) const
{
#ifdef USE_PROFILE
    if(profile_mode) {
        o << fDesc << ": " << elapsed*1000 << " msecs "
            << count << " times";
        if (count > 1)
            o << " " << (elapsed/count)*1000 << " msecs each\n";
    }
#endif
}

inline void RealTimer::BufPrint(std::ostream& o, std::string& str) const
{
#ifdef USE_PROFILE
    if(profile_mode) {
        o << str << ": " << elapsed*1000 << " msecs "
            << count << " times";
        if (count > 1)
            o << " " << (elapsed/count)*1000 << " msecs each\n";
    }
#endif
}
} // namespace
#endif //  COUTIL_GLOBAL_REALTIMER
