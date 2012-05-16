#include "DateTime.h"

DateTime::DateTime()
	:year(0),
	month(0),
	day(0),
	hour(0),
	minute(0),
	second(0),
	second_part(0),
	neg(0),
	reserve(0)
{
}

DateTime::DateTime(const char* str)
{
	Parse(str);
}

DateTime::DateTime(unsigned int y,unsigned int m,unsigned int d
		,unsigned int h,unsigned int mm,unsigned int s)
	:year(y),
	month(m),
	day(d),
	hour(h),
	minute(mm),
	second(s),
	second_part(0),
	neg(0),
	reserve(0)
{
}

DateTime::DateTime(const DateTime& value)
{
	memcpy(this,&value,sizeof(DateTime));
}


DateTime::operator tm () const
{
	tm t;
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;
	t.tm_isdst = 0;
	t.tm_wday = 0;
	t.tm_yday = 0;

	return t;
}

DateTime DateTime::operator+(const double& timespan) const
{
	DateTime dt;	

	tm start = (tm)(*this);

	time_t td = mktime(&start);

	td += (time_t)timespan;

	tm* result = localtime(&td);

	dt.year = result->tm_year + 1900;
	dt.month = result->tm_mon + 1;
	dt.day =  result->tm_mday;
	dt.hour = result->tm_hour;
	dt.minute = result->tm_min;
	dt.second = result->tm_sec;

	return dt;
}

double DateTime::operator-(const DateTime& rhs) const
{
	tm end = (tm)(*this);
	tm start = (tm)(rhs);
	
	return static_cast<double>(mktime(&end) - mktime(&start));
}


bool DateTime::operator==(const DateTime& rhs) const
{
	return memcmp(this,&rhs,sizeof(DateTime))==0;
}

bool DateTime::operator>(const DateTime& rhs) const
{
	if(((*this)-rhs)>0) return true;

	tm t0 = (tm)(*this);
	tm t1 = (tm)*const_cast<DateTime*>(&rhs);

	if(memcmp(&t0,&t1,sizeof(tm))==0) return true;
	
	return false;
}

bool DateTime::operator<(const DateTime& rhs) const
{
	if(!((*this)>rhs)) return true;

	return false;
}

bool DateTime::operator>=(const DateTime& rhs) const
{
	if((*this)>rhs
		||(*this)==rhs) return true;

	return false;
}

bool DateTime::operator<=(const DateTime& rhs) const
{
	if((*this)<rhs
		||(*this)==rhs) return true;

	return false;
}

DateTime DateTime::Now()
{
	time_t t;
	time(&t);
	tm *lt = localtime(&t);

	DateTime dt;

	dt.year = lt->tm_year + 1900;
	dt.month = lt->tm_mon + 1;
	dt.day =  lt->tm_mday;
	dt.hour = lt->tm_hour;
	dt.minute = lt->tm_min;
	dt.second = lt->tm_sec;

	return dt;
}

void DateTime::Parse(const string& str)
{
	/*ptime pt(time_from_string(str));
	date d = pt.date();
	time_duration t = pt.time_of_day();
	
	year = d.year();
	month = static_cast<unsigned char>(d.month());
	day = static_cast<unsigned char>(d.day());
	hour = static_cast<unsigned char>(t.hours());
	minute = static_cast<unsigned char>(t.minutes());
	second = static_cast<unsigned char>(t.seconds());*/

}
//只是限制在1989-01-02 12:12:24 类型的，需要完全格式,慎用
void DateTime::SetDate(string& str)
{
	int count=0;
	string temp;
	for(int i=0;i<str.size();i++)
	{
		if(str[i]=='-'||str[i]==' '||str[i]==':')
		{
			if(count==0)
			year=atoi(temp.c_str());
			else if(count==1)
			month=atoi(temp.c_str());
			else if(count==2)
			day=atoi(temp.c_str());
			else if(count==3)
			hour=atoi(temp.c_str());
			else if(count==4)
			minute=atoi(temp.c_str());
			else if(count==5)
			second=atoi(temp.c_str());

			count++;
			temp.clear();
		}
		else
		{
			temp.push_back(str[i]);
		}
	}
	if(count==2)
	{
		day=atoi(temp.c_str());
	}
	if(count==5)
		second=atoi(temp.c_str());
}

string DateTime::StringDateTime() const
{
	char v[20];
	sprintf(v,"%4d%02d%02d %02d:%02d:%02d",
		year, month, day,
		hour, minute, second);

	return string(v);
}

string DateTime::StringDate() const
{
	char v[20];
	sprintf(v,"%4d%02d%02d", year, month, day);

	return string(v);
}


void DateTime::SetTime(double ticks)
{
	if(ticks<0) return;

	DateTime td(1970,1,1);
	
	(*this) = td.Add(ticks,1);;
}

double DateTime::GetTime() const
{
	DateTime td(1970,1,1);
	tm start = (tm)(td);
	tm end = (tm)(*this);

	return static_cast<double>(mktime(&end)-mktime(&start));
}

DateTime DateTime::AddYears(int years)
{
	return AddMonths(years*12);
}

DateTime DateTime::AddMonths(int months)
{
	DateTime dt = (*this);

	int m = (month - 1) + months;
	if (m >= 0)
	{
		dt.month = (m % 12) + 1;
		dt.year += m / 12;
	}
	else
	{
		dt.month = 12 + ((m + 1) % 12);
		dt.year += (m - 11) / 12;
	}
	
	unsigned int days = DaysInMonth(dt.year, dt.month);

	if (dt.day > days)
	{
		dt.day = days;
	}

	return dt;
}

DateTime DateTime::AddDays(double days)
{
	return Add(days,86400*ticks_per_second);
}

DateTime DateTime::AddHours(double hours)
{
	return Add(hours,3600*ticks_per_second);
}

DateTime DateTime::AddMinutes(double minutes)
{
	return Add(minutes,60*ticks_per_second);
}

DateTime DateTime::AddSeconds(double seconds)
{
	return Add(seconds, 1*ticks_per_second);
}

DateTime DateTime::AddMilliseconds(double milliseconds)
{
	return Add(milliseconds, ticks_per_second/1000);
}

DateTime DateTime::Add(double value,long long scale)
{
	double num = static_cast<double>(value * scale);
	 
	return (*this) + num;
}
bool DateTime::IsLeepYear(int year)
{
	if ((year % 4) != 0)
	{
		return false;
	}
	if ((year % 100) == 0)
	{
		return ((year % 400) == 0);
	}
	return true;
}

int DateTime::DaysInMonth(int year, int month)
{
	static int DaysToMonth365[] = { 0, 0x1f, 0x3b, 90, 120, 0x97, 0xb5, 0xd4,
	0xf3, 0x111, 0x130, 0x14e, 0x16d };
	static int DaysToMonth366[] = { 0, 0x1f, 60, 0x5b, 0x79, 0x98, 0xb6, 0xd5, 
		0xf4, 0x112, 0x131, 0x14f,0x16e };

	int *numArray = IsLeepYear(year)?DaysToMonth366:DaysToMonth365;
	return (numArray[month] - numArray[month - 1]);
}

