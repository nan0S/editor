internal date_time
TimestampToDateTime(timestamp64 Ts)
{
 date_time Dt = {};
 Dt.Ms = Ts % 1000;
 Ts /= 1000;
 Dt.Sec = Ts % 60;
 Ts /= 60;
 Dt.Mins = Ts % 60;
 Ts /= 60;
 Dt.Hour = Ts % 24;
 Ts /= 24;
 Dt.Day = Ts % 31;
 Ts /= 31;
 Dt.Month = Ts % 12;
 Ts /= 12;
 Dt.Year = SafeCastU32(Ts);
 
 return Dt;
}

internal timestamp64
DateTimeToTimestamp(date_time Dt)
{
 timestamp64 Ts = 0;
 Ts += Dt.Year;
 Ts *= 12;
 Ts += Dt.Month;
 Ts *= 31;
 Ts += Dt.Day;
 Ts *= 24;
 Ts += Dt.Hour;
 Ts *= 60;
 Ts += Dt.Mins; 
 Ts *= 60;
 Ts += Dt.Sec;
 Ts *= 1000;
 Ts += Dt.Ms;
 
 return Ts;
}

internal exit_code_int
OS_CombineExitCodes(exit_code_int CodeA, exit_code_int CodeB)
{
 exit_code_int Result = 0;
 if (CodeA) Result = CodeA;
 else Result = CodeB;
 return Result;
}