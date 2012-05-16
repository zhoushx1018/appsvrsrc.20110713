#ifndef DATETIME_H
#define DATETIME_H

#include <stdio.h>
#include <memory>
#include <sys/timeb.h>
#include <time.h>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace std;

class DateTime
{
public:
	DateTime();

	DateTime(const char* str);

	DateTime(unsigned int year,unsigned int month,unsigned int day
		,unsigned int hour=0,unsigned int minute=0,unsigned int second=0);

	DateTime(const DateTime& value);

	operator tm () const;
	
	DateTime operator+(const double& timespan) const;

	double operator-(const DateTime& rhs) const;

	bool operator==(const DateTime& rhs) const;

	bool operator>(const DateTime& rhs) const;

	bool operator<(const DateTime& rhs) const;

	bool operator>=(const DateTime& rhs) const;

	bool operator<=(const DateTime& rhs) const;

	void Parse(const string& str);

	void SetDate(string& str);

	static DateTime Now();

	string StringDateTime() const;

	string StringDate() const;

	void SetTime(double ticks);

	double GetTime() const;

	DateTime AddYears(int years);

	DateTime AddMonths(int months);

	DateTime AddDays(double days);

	DateTime AddHours(double hours);

	DateTime AddMinutes(double minutes);

	DateTime AddSeconds(double seconds);

	DateTime AddMilliseconds(double milliseconds);

private:
	DateTime Add(double value,long long scale);

	static bool IsLeepYear(int year);

	static int DaysInMonth(int year, int month);

	static const long long ticks_per_second = 1;//1000000;
public:
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
	unsigned long second_part;
	unsigned int neg;
	unsigned int reserve;
};

#endif
