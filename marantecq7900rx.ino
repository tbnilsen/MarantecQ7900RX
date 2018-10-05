//*************************************************************************************************************
// Marantec Q7900 Garage Door Opener - RECEIVER
// Get a remote's key via a Patricle Photon
//
// written by:  Terje B. Nilsen
// version:     1.0
// date:        05 OCT 2018
//
// This code receives a 24-bit code via a 315MHz receiver card from Marantec Q7900 Garage Door Opener (Remote).
// This obtains the keycode that can be used with the MarantecQ7900 TX firmware
// This has only been verified to work with a Marantec Q7900 opener.
//*************************************************************************************************************

//*************************************************************************************************
//The TX bit rate for the Q7900 is 250Hz
//The transmission starts with a low idle of approximatley 12ms
//This idle period is also performed between repeated transmissions
//After that a steam of 24 bits are sent. Each bit cell (i.e. 4000uSec) has a single positive pulse 
//at the start. A long pulse (1500uSec) I consider a '1'. A short (750uSec) is a '0'
//*************************************************************************************************
#define BITFREQ  4000  //uSec
#define ONEPULSE 1500  //uSec
#define ZEROPULSE 750  //uSec
#define RXLENGTH   24  //number of bits to receive from the garage door remote
#define TXDELAY_LO  12000 //uSec of the delay before/between sent key sequences - longest
#define TXDELAY_HI  13000 //uSec of the delay before/between sent key sequences - shortest

#define RX_PIN D1 //the 315MHz receiver pin is connected to the Photon pin D1

#define MAXRX 100
int rxq; //zero if received pulse train looks good
int rxindex; //index into RX array
uint32_t rxuSecs[MAXRX]; //holds delta time in uSecs between pulse edges
uint32_t rxkey; //received remote 24bit key sequence (1st bit received is LSBit)
uint32_t rxmask; //24bit mask for or'ing in '1's
volatile int keyReady; //goes true when we have received a key

uint32_t bit_cutoff; 

void setup() 
{
    Serial.begin(9600);
    bit_cutoff = ( ONEPULSE +  ZEROPULSE ) / 2;
    rxmask = 1 << (RXLENGTH-1);
    delay(1000); //give the PC serial monitor a chance to load
    Serial.println("\nMarantecQ7900 - RX Program Start");
    //**************************************
    //Set RX pin and attach to the inetrrupt
    //**************************************
    pinMode(RX_PIN,INPUT);
    attachInterrupt(RX_PIN,rxISR,CHANGE); //interrupt on either edge
}

void loop() 
{
    //***************************
    //See if we've received a key
    //***************************
    if (keyReady)
    {
        rxkey = 0; //reset key
        rxq = 0;
        //We have rx'ed a key - process and print
        for(int i=0; i<RXLENGTH*2; i+=2)
        {
            //Serial.printf("%d,%d,",rxuSecs[i],rxuSecs[i+1]);
            rxkey >>= 1; //shift down by one
            if (rxuSecs[i] > bit_cutoff)
            {
                rxkey |= rxmask; //rx a '1'
            }
            //*************************
            //Check quality of bit cell
            //*************************
            if ((rxuSecs[i]+rxuSecs[i+1]) > (BITFREQ+(ZEROPULSE/2)) || (rxuSecs[i]+rxuSecs[i+1]) < (BITFREQ-(ZEROPULSE/2)))
            {
                rxq++; //poor quality
            }
        }
        keyReady = 0; //get ready to receive another
        Serial.printlnf("\n*** Key received: 0x%x  ***",rxkey);
        if (!rxq)
        {
            Serial.printlnf("*** Good Quality! ***");
        }
        else
        {
            Serial.printlnf("Iffy Quality (number of questionable bit cells = %d).  TRY AGAIN.",rxq);
        }
    }
}

//**************************************************************
//Receiver Interrupt Service Routine.
//
//Don't be surprised if the receiver is constantly 
//interrupting the Photon. The RX's are sensitive and suceptible
//to noise. I ignore this spurious activity
//**************************************************************
void rxISR() 
{
    uint32_t now = micros(); //grab time
    static int rxstate = 0;
    static uint32_t lastTime = 0;
    uint32_t delta = now - lastTime;

    lastTime = now; //keep up to date
    
    switch(rxstate)
    {
    case 0: //waiting for a low and background has processed a pending key
        if (!digitalRead(RX_PIN) && !keyReady)
        {
            rxstate++; //go to next state
        }
        break;
    case 1: //check if low period is within idle range
        if (delta >= TXDELAY_LO && delta <= TXDELAY_HI)
        {
            rxstate++; //go to next state
            rxindex = 0;
        }
        else
        {
            rxstate = 0; //go back
        }
        break;
    case 2: //receive bits
        rxuSecs[rxindex++] = delta;
        if (rxindex >= (2*RXLENGTH)-1)
        {//we won't receive the last idle - it becomes part of the next sequence (hence the minus1)
            rxuSecs[rxindex] = BITFREQ - delta; //allow quality detector to work well
            keyReady = 1; //we have received a 24bit code
            rxstate = 0; //go back
        }
        break;
    }
}
