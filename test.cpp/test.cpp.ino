#define MINIMUM_FILL 1
#define MAXIMUM_FILL 20

#define SECONDS_IN_ONE_WEEK 604800
#define SECONDS_IN_ONE_DAY 86400
#define SECONDS_IN_ONE_HOUR 3600
#define SECONDS_IN_ONE_MINUTE 60
#define TIME_OF_DAILY_MESSAGE 16

bool fillPushMessageSentFlags = false;
bool oneHourPushMessageSentFlags = false;
bool dailyPushMessageSent = false;
bool checkIfSendPushMessage = false;
long testOneHourTimer = 0;

void setup()
{
    Serial.begin(9600);
}

//checked tests
void checkIfSendDailyPushMessage(long testTime){
    if((testTime  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == 7)
        ::dailyPushMessageSent = false;
    if((testTime  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == TIME_OF_DAILY_MESSAGE && dailyPushMessageSent == false){
        ::dailyPushMessageSent = true;
        ::checkIfSendPushMessage = true;
    }
}  

void startOneHourTimerIfEmpty(float currentFill, long testTime){
    if(currentFill <= MINIMUM_FILL &&  testOneHourTimer == 0){
        ::testOneHourTimer = testTime + SECONDS_IN_ONE_HOUR;
        ::fillPushMessageSentFlags = false;
        Serial.print("One hour timer started at ");
        printTime(testTime);
    }
    else if(currentFill > MINIMUM_FILL){
        ::testOneHourTimer = 0;
        ::oneHourPushMessageSentFlags = false;
    }
}

void checkIfSendEmptyPushMessage(float currentFill, long testTime){
    if(currentFill < MINIMUM_FILL && oneHourPushMessageSentFlags == false && testOneHourTimer <= testTime){
        ::oneHourPushMessageSentFlags = true;
        ::checkIfSendPushMessage = true;
    }
}   
 
bool checkIfSendFillPushMessage(float currentFill){
    if(currentFill > MINIMUM_FILL && currentFill < MAXIMUM_FILL && fillPushMessageSentFlags == false){
        ::fillPushMessageSentFlags = true;
        ::checkIfSendPushMessage = true;
    }
} 

void loop()
{
    dailyMessageTest();
    Serial.println("-------------------------");
    emptyMessageTest();
    Serial.println("-------------------------");
    fillMessageTest();
    delay(5000);
    exit(0);
    
}

 void dailyMessageTest()
 {
    int correct = 0, incorrect = 0;
    Serial.println("Daily message test:");
    for (long testTime = 0; testTime < SECONDS_IN_ONE_WEEK; ++testTime){
        ::checkIfSendPushMessage = false;
        checkIfSendDailyPushMessage(testTime);
        correct = checkCorrectDaily(testTime, correct);
        incorrect = checkIncorrectDaily(testTime, incorrect);
    }
    Serial.print("Correct tests: ");
    Serial.println(correct);
    Serial.print("Incorrect tests: ");
    Serial.println(incorrect);
}

 void emptyMessageTest()
 {
    int correct = 0, incorrect = 0;
    float testCurrentFill = 100.0f;
    Serial.println("Empty message test:");
    for (long testTime = 0; testTime <= SECONDS_IN_ONE_DAY; ++testTime){
        ::checkIfSendPushMessage = false;
        if(testTime == 18000 || testTime == 54000)
            testCurrentFill = 0.0f;
        else if(testTime == 36000 || testTime == 72000)
          testCurrentFill = 100.0f;
        startOneHourTimerIfEmpty(testCurrentFill, testTime);
        checkIfSendEmptyPushMessage(testCurrentFill, testTime);
        correct = checkCorrectEmpty(testTime, testCurrentFill, correct);
        incorrect = checkIncorrectEmpty(testTime, testCurrentFill, incorrect);
    }
    Serial.print("Correct tests: ");
    Serial.println(correct);
    Serial.print("Incorrect tests: ");
    Serial.println(incorrect);
}

void fillMessageTest()
 {
  int correct = 0, incorrect = 0;
  Serial.println("Fill message test:");
  for (float testCurrentFill = 100.0f; testCurrentFill >= 0; --testCurrentFill){
      if(testCurrentFill == 15 || testCurrentFill == 10 || testCurrentFill == 5)
          fillPushMessageSentFlags = false;
      ::checkIfSendPushMessage = false;
      checkIfSendFillPushMessage(testCurrentFill);
      correct = checkCorrectFill(testCurrentFill, correct);
      incorrect = checkIncorrectFill(testCurrentFill, incorrect);
  }
  Serial.print("Correct tests: ");
  Serial.println(correct);
  Serial.print("Incorrect tests: ");
  Serial.println(incorrect);
}

int checkCorrectDaily(long testTime,  int correct)
{
    if (::checkIfSendPushMessage == true  && (testTime  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == TIME_OF_DAILY_MESSAGE){
        ++correct;
        Serial.print("Correct daily message sent at: ");
        printTime(testTime);
    }
    return correct;
}

int checkIncorrectDaily(long testTime, int incorrect)
{
    if(::checkIfSendPushMessage == true  && (testTime  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR != TIME_OF_DAILY_MESSAGE){
        ++incorrect;
        Serial.print("Incorrect daily message sent at: ");
        printTime(testTime);
    }
    return incorrect;
}

int checkCorrectEmpty(long testTime, float testCurrentFill,  int correct)
{
    if(::checkIfSendPushMessage == true && testCurrentFill <= MINIMUM_FILL && oneHourPushMessageSentFlags == true && testOneHourTimer <= testTime){
        ++correct;
        Serial.print("Correct empty message sent at: ");
        printTime(testTime);
        Serial.print("Current fill: ");
        Serial.println(testCurrentFill);
    }
    return correct;
}

int checkIncorrectEmpty(long testTime, float testCurrentFill,  int incorrect)
{
    if(::checkIfSendPushMessage == true && (testCurrentFill > MINIMUM_FILL || oneHourPushMessageSentFlags == false || testOneHourTimer > testTime)){
        ++incorrect;
        Serial.print("Incorrect empty message sent at: ");
        printTime(testTime);
        Serial.print("Current fill: ");
        Serial.println(testCurrentFill);
    }
    return incorrect;
}

int checkCorrectFill(float testCurrentFill,  int correct)
{
    if(checkIfSendPushMessage == true && testCurrentFill > MINIMUM_FILL && testCurrentFill < MAXIMUM_FILL && fillPushMessageSentFlags == true){
        ++correct;
        Serial.print("Correct current fill: ");
        Serial.println(testCurrentFill);
    }
    return correct;
}

int checkIncorrectFill(float testCurrentFill,  int incorrect)
{
    if(checkIfSendPushMessage == true && (testCurrentFill <= MINIMUM_FILL || testCurrentFill >= MAXIMUM_FILL || fillPushMessageSentFlags == false)){
        ++incorrect;
        Serial.print("Incorrect current fill: ");
        Serial.println(testCurrentFill);
    }
    return incorrect;
}

void printTime(long testTime){
    String timeMessage = ("day ");
    timeMessage += (testTime  / SECONDS_IN_ONE_DAY + 1);
    timeMessage += (" ");
    timeMessage += (testTime  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR;
    timeMessage += ":";
    if (((testTime % SECONDS_IN_ONE_HOUR) / SECONDS_IN_ONE_MINUTE) < 10)
        timeMessage += "0";
    timeMessage += (testTime  % SECONDS_IN_ONE_HOUR) / SECONDS_IN_ONE_MINUTE;
    timeMessage += ":";
    if ((testTime % SECONDS_IN_ONE_MINUTE) < 10) {
        timeMessage += "0";
    }
    timeMessage += testTime % SECONDS_IN_ONE_MINUTE;
    Serial.println(timeMessage); 
}
