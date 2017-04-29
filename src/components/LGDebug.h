
#ifndef _LGDEBUG_H_
#define _LGDEBUG_H_

#include <sys/time.h>
#include <stdio.h>


// The following is cribbed from the GCC manual:

/* Subtract the `struct timeval' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);	
		
class CPUBench
{
public:
	CPUBench()
	{
		gettimeofday(&m_start, 0);
	}
	
	~CPUBench()
	{
		usec_t curTime;

		gettimeofday(&m_finish, 0);
		
		struct timeval diff;
		timeval_subtract(&diff, &m_finish, &m_start);
		curTime = (diff.tv_sec*1000000) + diff.tv_usec;
				
		if (s_peak < curTime)
			s_peak = curTime;
		
		s_sum += curTime;
		s_numBufs++;
	}
	
	// Reset the value for a new frame
	void Reset()
	{
		s_sum = 0;
		s_numBufs = 0;
		s_peak = 0;
	}
	
	// Print out the CPU% given the length of the buffer
	static void Print(float bufUs)
	{
		printf("Average: %02.2f;  Peak: %02.2f\n", (((float)s_sum/(float)s_numBufs)/bufUs)*100, ((float)s_peak/bufUs)*100);
		// It's kind of weird to set lastAverage here, but it's convinient
		s_lastAverage = (((float)s_sum/(float)s_numBufs)/bufUs)*100;
	}
	
	static float GetLastAverage() { return s_lastAverage; }
	
private:
	typedef long int usec_t;  
	
	// Beginning and end of the interval currently measured
	struct timeval m_start;
	struct timeval m_finish;
	
	// Running sum of all the buffers so far
	static usec_t s_sum;
	// Number of buffers in sum
	static int s_numBufs;
	
	// Peak - worst buffer length so far
	static usec_t s_peak;
	
	static float s_lastAverage;
};

#endif
