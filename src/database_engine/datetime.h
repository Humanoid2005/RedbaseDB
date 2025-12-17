#ifndef DATETIME_H
#define DATETIME_H

#include <iostream>

class DateTime{
    public:
        int day;
        int month;
        int year;
        int hour;
        int minute;
        int second;
    
        DateTime(int day=0,int month=0,int year=0,int hour=0,int minute=0,int second=0){
            this->day = day;
            this->month = month;
            this->year = year;
            this->hour = hour;
            this->minute = minute;
            this->second = second;
        }
    
        bool operator==(const DateTime &other) const{
            return this->day==other.day && this->month==other.month && this->year==other.year &&
                   this->hour==other.hour && this->minute==other.minute && this->second==other.second;
        }
        bool operator!=(const DateTime &other) const{
            return this->day!=other.day || this->month!=other.month || this->year!=other.year ||
                   this->hour!=other.hour || this->minute!=other.minute || this->second!=other.second;
        }
        long long int getTimestamp() const{
            return (this->year*365*24*60*60) + (this->month*30*24*60*60) + (this->day*24*60*60) +
                   (this->hour*60*60) + (this->minute*60) + this->second;
        }

        static std::string DTtoString(DateTime dt){
            std::string datetime  = std::to_string(dt.day)+"-"+std::to_string(dt.month)+"-"+std::to_string(dt.year)+" "+std::to_string(dt.hour)+":"+std::to_string(dt.minute)+":"+std::to_string(dt.second);
            return datetime;
        }

        bool operator<(const DateTime &other) const{
            return this->getTimestamp() < other.getTimestamp();
        }

        bool operator>(const DateTime &other) const{
            return this->getTimestamp() > other.getTimestamp();
        }

        bool operator<=(const DateTime &other) const{
            return this->getTimestamp() <= other.getTimestamp();
        }

        bool operator>=(const DateTime &other) const{
            return this->getTimestamp() >= other.getTimestamp();
        }

        friend std::ostream &operator<<(std::ostream &out, const DateTime &dt) {
            out << dt.day << "-" << dt.month << "-" << dt.year << " " << dt.hour << ":" << dt.minute << ":" << dt.second;
            return out;
        }
        
        friend std::istream &operator>>(std::istream &in, DateTime &dt) {
            char ch;
            in >> dt.day >> ch >> dt.month >> ch >> dt.year >> ch >> dt.hour >> ch >> dt.minute >> ch >> dt.second;
            return in;
        }
    };
    
    inline bool validateDateTime(int day, int month, int year, int hour, int minute, int second){
        if(day<1 || day>31 || month<1 || month>12 || year<0 || hour<0 || hour>23 || minute<0 || minute>59 || second<0 || second>59){
            return false;
        }
        if(month==2 && day>29){
            return false;
        }
        if((month==4 || month==6 || month==9 || month==11) && day>30){
            return false;
        }
        return true;
    }
    
    inline bool validateDate(int day, int month, int year){
        return validateDateTime(day, month, year, 0, 0, 0);
    }

    inline bool validateTime(int hour, int minute, int second){
        return validateDateTime(0, 0, 0, hour, minute, second);
    }

#endif