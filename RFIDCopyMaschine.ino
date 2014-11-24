/*
*	Some example code how to copy a card with known keys
*	Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
*	by ebc ( 2014) http://ebc81.wordpress.com/
*   V0.1
*-----------------------------------------------------------------------------
* Pin layout should be as follows:
* Signal     Pin              Pin               Pin
*            Arduino Uno#     Arduino Mega      MFRC522 board
* ------------------------------------------------------------
* Reset      9                5                 RST
* SPI SS     10               53                SDA
* SPI MOSI   11               51                MOSI
* SPI MISO   12               50                MISO
* SPI SCK    13               52                SCK
*
*  #Note: Code will work not with Arduino UNO because of not enough SRAM 
*		  Code will work with Mifare Classic 1K Card compatibles, for other code changes in code may necessary
*/
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 53    //Arduino Mega 2560
#define RST_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);        // Create MFRC522 instance.

MFRC522::MIFARE_Key keyEmpty;
MFRC522::MIFARE_Key keyA;
MFRC522::MIFARE_Key keyB;

MFRC522::Uid LastReadUid;

#define PRG_MODE_NONE				(0)
#define PRG_MODE_READ				(1)
#define PRG_MODE_READ_SPECIFIC		(2)
#define PRG_MODE_READ_BLANK			(3)
#define PTG_MODE_WRITE				(4)
byte  prg_modus = PRG_MODE_NONE ;
bool  have_dump = false;
byte selected_sector = 0;
byte selected_block  = 0;
bool selected_useKeyB = false;


int led = 13;

// Mifare Classic 1K Card
#define MAX_SECTOR			(16)		// Sektoren
#define MAX_BLOCK_SECTOR	 (4)		// Blöcke pro Sektoren
#define MAX_DATA_BLOCK		(16)		// Bytes pro Block
byte mydumpdata[MAX_SECTOR][MAX_BLOCK_SECTOR][MAX_DATA_BLOCK];

//-----------------------------------------------------------------
// KNOWN KEYS FROM CARD
//-----------------------------------------------------------------
byte KeyA_List[][6] =
{
	{150,161,162,166,164,165},  // Sector 0
	{238, 51,226, 88, 63, 28},  // Sector 1
	{220,140,161, 12,125, 89},  // Sector 2
	{113, 97,186,136,120,156},  // Sector 3
	{112, 17,126,250,164, 50},  // Sector 4
	{212, 24,175, 51,161,155},  // Sector 5
	{139,192,187,206, 50,233},  // Sector 6
	{156, 84,200,238,139, 28},  // Sector 7
	{141, 53,120, 10, 20,141},  // Sector 8
	{178,237,163,146,170, 83},  // Sector 9
	{137,254,143,198, 79,169},  // Sector 10
	{194,105,111,160,234,209},  // Sector 11
	{264,152,043,143, 62,226},  // Sector 12
	{131,232,181,154, 81,224},  // Sector 13
	{174,  1,208,226, 63,140},  // Sector 14
	{140,161,162,163,164,165},  // Sector 15
};

byte KeyB_List[][6] =
{
	{134,135,136,137,138,139},  // Sector 0
	{146,147,148,149,140,141},  // Sector 1
	{156,157,158,159,150,151},  // Sector 2
	{166,167,178,179,180,181},  // Sector 3
	{186,187,178,179,180,181},  // Sector 4
	{140,147, 15, 51,241,  7},  // Sector 5
	{235,  8,234, 57,122,154},  // Sector 6
	{128,132, 14,  7,165,143},  // Sector 7
	{146,157,178,179,180,181},  // Sector 8
	{156,167,178,179,180,181},  // Sector 9
	{166,177,178,179,180,181},  // Sector 10
	{186,187,178,179,180,181},  // Sector 11
	{185, 19,165,113,135,140},  // Sector 12
	{176,148, 45,181,196,175},  // Sector 13
	{235, 15, 14,122,141,213},  // Sector 14
	{116,140,255, 47, 27,199},  // Sector 15
};
//---------------------------------------------------------



//---------------------------------------------------------
// Example: Dynamic KeyB generation based on the UID of a card
//---------------------------------------------------------
void CalculateKeyBDynamisch(MFRC522::Uid *SerNr)
{
	byte array[4];

	array[0] = SerNr->uidByte[0];
	array[1] = SerNr->uidByte[1];
	array[2] = SerNr->uidByte[2];
	array[3] = SerNr->uidByte[3];

	// some fancy generation code..
	// i have my own :-)
	//...
	for ( int sector = 0; sector < MAX_SECTOR; sector++ )
	{
		KeyB_List[sector][0] = array[0] ^ KeyB_List[sector][0];
		KeyB_List[sector][2] = array[0] ^ KeyB_List[sector][2];
	}

	Serial.println("----------------New KeyB--------------------------------------");
	for ( int sector = 0; sector < MAX_SECTOR; sector++ )
	{
		Serial.print("Sector: ");Serial.print(sector);Serial.print(" ");
		Serial.print(KeyB_List[sector][0]);Serial.print(" ");
		Serial.print(KeyB_List[sector][1]);Serial.print(" ");
		Serial.print(KeyB_List[sector][2]);Serial.print(" ");
		Serial.print(KeyB_List[sector][3]);Serial.print(" ");
		Serial.print(KeyB_List[sector][4]);Serial.print(" ");
		Serial.print(KeyB_List[sector][5]);Serial.println(" ");
	}
}


/**
* Print key via serial interface
*/
void myprintKey(MFRC522::MIFARE_Key *key)
{
	Serial.print("Key;");
	if ( key->keyByte[0] < 10 )  Serial.print(" ");
	if ( key->keyByte[0] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[0]);
	Serial.print(" ");
	if ( key->keyByte[1] < 10 )  Serial.print(" ");
	if ( key->keyByte[1] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[1]);
	Serial.print(" ");
	if ( key->keyByte[2] < 10 )  Serial.print(" ");
	if ( key->keyByte[2] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[2]);
	Serial.print(" ");
	if ( key->keyByte[3] < 10 )  Serial.print(" ");
	if ( key->keyByte[3] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[3]);
	Serial.print(" ");
	if ( key->keyByte[4] < 10 )  Serial.print(" ");
	if ( key->keyByte[4] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[4]);
	Serial.print(" ");
	if ( key->keyByte[5] < 10 )  Serial.print(" ");
	if ( key->keyByte[5] < 100 ) Serial.print(" ");
	Serial.print(key->keyByte[5]);
	Serial.print(";");
}

void PrintSerialMenuText()
{
	Serial.println("");
	Serial.println("Press (A) ReadCard / (B) Write / (C) DumpOut / (D)ReadBlank / (E)ReadSpecificSector / (F)Calc dynamic KeyB");
}



/**
* Arduino Setup
*/
void setup() 
{
	Serial.begin(57600);		// Initialize serial communications with the PC20
	
	Serial.println("Welcome to RFID CopyMaschine");
	SPI.begin();				// Init SPI bus
	mfrc522.PCD_Init();			// Init MFRC522 card
	pinMode(led, OUTPUT);
	
	// Init empty card key
	memset(&keyEmpty,0xFF,sizeof(keyEmpty));
	Serial.print("Default Key for empty card ");
	myprintKey(&keyEmpty);
	PrintSerialMenuText();
}


/**
* CheckAccessBits
* see Data sheet 8.6.3 Sector trailer (block 3) / 8.7.1 Access conditions ( Fig 9. Access conditions)
* buffer must at least 9 and g must be 4 - no check !
*/
bool CalcAccessBits(byte *buffer, byte *g)
{
	// The access bits are stored in a peculiar fashion.
	// There are four groups:
	//		g[3]	Access bits for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
	//		g[2]	Access bits for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
	//		g[1]	Access bits for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
	//		g[0]	Access bits for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)
	// Each group has access bits [C1 C2 C3]. In this code C1 is MSB and C3 is LSB.
	// The four CX bits are stored together in a nible cx and an inverted nible cx_.
	byte c1, c2, c3;		// Nibbles
	byte c1_, c2_, c3_;		// Inverted nibbles
	bool invertedError;		// True if one of the inverted nibbles did not match
	//byte g[4];				// Access bits for each of the four groups.
	
	c1  = buffer[7] >> 4;
	c2  = buffer[8] & 0xF;
	c3  = buffer[8] >> 4;
	c1_ = buffer[6] & 0xF;
	c2_ = buffer[6] >> 4;
	c3_ = buffer[7] & 0xF;
	invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
	g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
	g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
	g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
	g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
	
	return invertedError;
}

/**
* Check if card is empty
* Access via default empty key
* All bytes = 0 on card ( except uid )
*/
bool CheckCardIsEmpty(bool withOutput)
{
	byte status;
	byte buffer[18];	// buffer for receiving data from card
	byte g[4];			// Access bits for each of the four groups.
	
	prg_modus= PRG_MODE_NONE; // in every return case the next status should be chosen by operator
	
	Serial.println("CheckCardIsEmpty ...");
	for (int sector = (MAX_SECTOR-1); sector >= 0; sector-- )
	{
		for ( int block = (MAX_BLOCK_SECTOR-1); block >= 0; block-- )
		{
			if ( withOutput )
			{
				if ( block == 3 )
				{
					Serial.print(sector < 10 ? "   " : "  "); // Pad with spaces
					Serial.print(sector);
					Serial.print("   A");
					myprintKey(&keyEmpty);
					//Serial.print("   B");
					//myprintKey(&keyB);
					Serial.println("");
				}
			
				Serial.print("       ");
			
				// Block Adresse
				Serial.print((sector*4+block) < 10 ? "   " : ((sector*4+block) < 100 ? "  "	 : " ")); // Pad with spaces
				Serial.print((sector*4+block));
				Serial.print("  ");
			}
			// Establish encrypted communications before reading the first block
			if ( block == 3 )
			{
				status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (sector*4)+block, &keyEmpty, &(mfrc522.uid));
				if (status != MFRC522::STATUS_OK) 
				{
					Serial.println("");
					Serial.print("CheckCardIsEmpty: PCD_Authenticate KEY A() failed: ");
					Serial.println(mfrc522.GetStatusCodeName(status));
					Serial.println("-- Card not empty --");
					return false;
				}
			}
			// Read block
			byte byteCount = sizeof(buffer);
			status = mfrc522.MIFARE_Read( (sector*4)+block, buffer, &byteCount);
			if (status != MFRC522::STATUS_OK) 
			{
				Serial.println("");
				Serial.print("Sector ");Serial.print(sector);Serial.print(" Block "),Serial.print(block);
				Serial.print(" ---CheckCardIsEmpty: MIFARE_Read() failed: ");
				Serial.println(mfrc522.GetStatusCodeName(status));
				// necessary ? 
				mfrc522.PICC_HaltA(); // Halt PICC
				mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
				return false;
			}
			if ( block == 3 )
			{
				if ( CalcAccessBits(buffer,g))
				{
					Serial.println("");
					Serial.println("Inverted access bits did not match!");
				}
			}
			
			for ( int index = 0; index < MAX_DATA_BLOCK; index++ )
			{
				if ( withOutput ) 
				{
					Serial.print(buffer[index] < 0x10 ? " 0" : " ");
					Serial.print(buffer[index], HEX);
					if ((index % 4) == 3) {
						Serial.print(" ");
					}
				}
				bool skip = false;
				// Skip 
				if ( sector == 0 && block == 0 )// there is the UID so skip
					skip = true;
				else if ( block == 3 && index >= 6 )// AccessBits
					skip = true;
				
				// Check if data is empty
				if ( !skip) 
				{
					if ( buffer[index] != 0 ) // data must be empty
					{
						Serial.println("");
						Serial.print("Card is not empty: Please use a empty card: Sector=");
						Serial.print(sector);Serial.print(" Block=");Serial.print(block);Serial.println(" ");
						mfrc522.PICC_HaltA(); // Halt PICC
						mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
						return false;
					}
				}
			}
			if ( withOutput )
			{
				// Print access bits
				Serial.print(" [ ");
				Serial.print((g[block] >> 2) & 1, DEC); Serial.print(" ");
				Serial.print((g[block] >> 1) & 1, DEC); Serial.print(" ");
				Serial.print((g[block] >> 0) & 1, DEC);
				Serial.print(" ] ");
				Serial.println("");
			}
		}
	}
	mfrc522.PICC_HaltA(); // Halt PICC
	mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
	Serial.println("Card is empty ..");
	return true;
}


/**
* Write previously dumped data to card
* Assumes that card is blank and has access key 0xFF
*/
void Write2EmptyCard()
{
	byte status;
	prg_modus= PRG_MODE_NONE;
	Serial.println("Write Card...");

	// First check if card is blank
	//if ( !CheckCardIsEmpty(false) )
	//	return;
 
	CalculateKeyBDynamisch(&mfrc522.uid);
	
	//mfrc522.PCD_Reset();
	/*
	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent())
	{
		LastReadUid.uidByte[0] = 0;
		Serial.println("Write2EmptyCard Cancel: Card not present");
		return;
	}
	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial())
	{
		LastReadUid.uidByte[0] = 0;
		Serial.println("Write2EmptyCard Cancel: PICC_ReadCardSerial failed");
		return;
	}*/
	
	for ( int sector = 0; sector < MAX_SECTOR; sector++ )
	{
		for ( int block = 0; block < MAX_BLOCK_SECTOR; block++ )
		{
			if ( sector == 0 && block == 0 ) //UID read only
				continue;

			// Establish encrypted communications before writing
			if (1)
			{
				status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (sector*4)+block, &keyEmpty, &(mfrc522.uid));
				if (status != MFRC522::STATUS_OK) 
				{
					Serial.print("PCD_Authenticate KEY A() failed: ");
					myprintKey(&keyEmpty);
					Serial.print("  sector=");Serial.print(sector);
					Serial.print("  block=");Serial.print(block);
					Serial.println(mfrc522.GetStatusCodeName(status));
					Serial.print("Card UID:");    //Dump UID
					for (byte i = 0; i < mfrc522.uid.size; i++)
					{
						Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
						Serial.print(mfrc522.uid.uidByte[i], HEX);
					}
					Serial.println("");
					break;
				}
				/*
				status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, (sector*4)+block, &keyEmpty, &(mfrc522.uid));
				if (status != MFRC522::STATUS_OK) 
				{
					Serial.print("PCD_Authenticate() KEY B()  failed: ");
					myprintKey(&keyEmpty);
					Serial.println(mfrc522.GetStatusCodeName(status));
					break;
				}*/
			}
			Serial.print("Sector ");Serial.print(sector);Serial.print(" ");
			Serial.print("Block ");Serial.print(block);Serial.print(" ");
			Serial.print("TotalBlock ");Serial.print((sector*4)+block);Serial.print(" ");
			
			// Assign new keyB
			if ( block == 3 )
			{
				mydumpdata[sector][block][10] = KeyB_List[sector][0];
				mydumpdata[sector][block][11] = KeyB_List[sector][1];
				mydumpdata[sector][block][12] = KeyB_List[sector][2];
				mydumpdata[sector][block][13] = KeyB_List[sector][3];
				mydumpdata[sector][block][14] = KeyB_List[sector][4];
				mydumpdata[sector][block][15] = KeyB_List[sector][5];
			}

			status = mfrc522.MIFARE_Write((sector*4)+block, &mydumpdata[sector][block][0], 16);
			if (status != MFRC522::STATUS_OK) 
			{
				Serial.print("MIFARE_Write() failed: ");
				Serial.println(mfrc522.GetStatusCodeName(status));
				for ( int i = 0; i < 16 ; i++ )
				{
					Serial.print(mydumpdata[sector][block][i] < 0x10 ? " 0" : " ");
					Serial.print(mydumpdata[sector][block][i], HEX);
					if ((i % 4) == 3) Serial.print(" ");
				}
				Serial.println("");
				break;
			}
			else
			{
				Serial.println("OK");
				for ( int i = 0; i < 16 ; i++ )
				{
					Serial.print(mydumpdata[sector][block][i] < 0x10 ? " 0" : " ");
					Serial.print(mydumpdata[sector][block][i], HEX);
					if ((i % 4) == 3) Serial.print(" ");
				}
				Serial.println("");
			}
		}
	}
	mfrc522.PICC_HaltA(); // Halt PICC
	mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
	prg_modus= PRG_MODE_NONE;
	Serial.println("end write operation...");
}

/**
* Read a card with known keys: 
* Define keys in KeyA_List, KeyB_List
* the read data are dumped into mydumpdata
* keyA and keyB data are added to the dump ( note: keyA can't read from card, must add manually) 
*/
void ReadCardwithKeys(bool useKeyB)
{
	ReadCardwithKeysSpecified((MAX_SECTOR-1),0,(MAX_BLOCK_SECTOR-1),0,useKeyB);
}

void ReadCardwithKeysSpecified(byte sector_von, byte sector_bis, byte block_von, byte block_bis, bool use_keyB )
{
	byte status;
	byte buffer[18];	// buffer for receiving data from card
	byte g[4];			// Access bits for each of the four groups.

	prg_modus= PRG_MODE_NONE; // in every return case the next status should be chosen by operator

	memset(mydumpdata,0,sizeof(mydumpdata));

	for (int sector = sector_von; sector >= sector_bis; sector-- )
	{
		// sector keys
		keyA.keyByte[0]		=	KeyA_List[sector][0];
		keyA.keyByte[1]		=	KeyA_List[sector][1];
		keyA.keyByte[2]		=	KeyA_List[sector][2];
		keyA.keyByte[3]		=	KeyA_List[sector][3];
		keyA.keyByte[4]		=	KeyA_List[sector][4];
		keyA.keyByte[5]		=	KeyA_List[sector][5];

		keyB.keyByte[0]		=	KeyB_List[sector][0];
		keyB.keyByte[1]		=	KeyB_List[sector][1];
		keyB.keyByte[2]		=	KeyB_List[sector][2];
		keyB.keyByte[3]		=	KeyB_List[sector][3];
		keyB.keyByte[4]		=	KeyB_List[sector][4];
		keyB.keyByte[5]		=	KeyB_List[sector][5];
		
		Serial.println("------------------------------------------");
		for ( int block = block_von; block >= block_bis; block-- )
		{
			if ( block == 3 || block_von == block_bis ) 
			{
				Serial.print(sector < 10 ? "   " : "  "); // Pad with spaces
				Serial.print(sector);
				Serial.print("   A");
				myprintKey(&keyA);
				if ( use_keyB ) 
				{
					Serial.print("   B");
					myprintKey(&keyB);
				}
				Serial.println("");
			}
			
			Serial.print("       ");
			
			// Block Adresse
			Serial.print((sector*4+block) < 10 ? "   " : ((sector*4+block) < 100 ? "  "	 : " ")); // Pad with spaces
			Serial.print((sector*4+block));
			Serial.print("  ");
			
			// Establish encrypted communications before reading the first block
			if ( block == 3 || block_von == block_bis) 
			{
				status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (sector*4)+block, &keyA, &(mfrc522.uid));
				if (status != MFRC522::STATUS_OK) 
				{
					Serial.println("");
					Serial.print("PCD_Authenticate() failed: KEY A ");
					Serial.println(mfrc522.GetStatusCodeName(status));
					have_dump = false;
					return;
				}
				if ( use_keyB ) 
				{
					status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, (sector*4)+block, &keyB, &(mfrc522.uid));
					if (status != MFRC522::STATUS_OK)
					{
						Serial.println("");
						Serial.print("PCD_Authenticate() failed: KEY B ");
						Serial.println(mfrc522.GetStatusCodeName(status));
						have_dump = false;
						return;
					}
				}
			}
			
			// Read block
			byte byteCount = sizeof(buffer);
			status = mfrc522.MIFARE_Read((sector*4)+block, buffer, &byteCount);
			if (status != MFRC522::STATUS_OK) 
			{
				Serial.println("");
				Serial.print("Sector ");Serial.print(sector);Serial.print(" Block "),Serial.print(block);
				Serial.print("  ---MIFARE_Read() failed: ");
				Serial.println(mfrc522.GetStatusCodeName(status));
				have_dump = false;
				continue;
			}
			else  // Dump data
			{
				if ( block == 3 )
				{
					if ( CalcAccessBits(buffer,g) )
					{
						Serial.println("");
						Serial.println("Inverted access bits did not match!");
					}
				}
				
				for (byte index = 0; index < MAX_DATA_BLOCK; index++) 
				{
					Serial.print(buffer[index] < 0x10 ? " 0" : " ");
					Serial.print(buffer[index], HEX);
					if ((index % 4) == 3) {
						Serial.print(" ");
					}
					// Dump data
					mydumpdata[sector][block][index] = buffer[index];
				}
				// Special note on KEY_A / KEY_B
				// see datasheet
				// When the sector trailer is read, the key bytes are blanked out by returning logical zeros. If
				// Key B is configured to be readable, the data stored in bytes 10 to 15 is returned, see
				if ( block == 3 )
				{
					// Key A will always 0, so fill dump  with the know KeyA otherwise read will write KeyA with zeros
					mydumpdata[sector][block][0] = keyA.keyByte[0];
					mydumpdata[sector][block][1] = keyA.keyByte[1];
					mydumpdata[sector][block][2] = keyA.keyByte[2];
					mydumpdata[sector][block][3] = keyA.keyByte[3];
					mydumpdata[sector][block][4] = keyA.keyByte[4];
					mydumpdata[sector][block][5] = keyA.keyByte[5];
					// For Key B access level should  checked
					// see Data sheet 8.7.2 Access conditions for the sector trailer / Table 6. Access conditions for the sector trailer
					//if ( (g[3] == 0) // C1/C2/C3 =  0
					// never tested
					mydumpdata[sector][block][10] = keyB.keyByte[0];
					mydumpdata[sector][block][11] = keyB.keyByte[1];
					mydumpdata[sector][block][12] = keyB.keyByte[2];
					mydumpdata[sector][block][13] = keyB.keyByte[3];
					mydumpdata[sector][block][14] = keyB.keyByte[4];
					mydumpdata[sector][block][15] = keyB.keyByte[5];
				}
				
				// Print access bits
				Serial.print(" [ ");
				Serial.print((g[block] >> 2) & 1, DEC); Serial.print(" ");
				Serial.print((g[block] >> 1) & 1, DEC); Serial.print(" ");
				Serial.print((g[block] >> 0) & 1, DEC);
				Serial.print(" ] ");
				Serial.println("");
			}
		}
		have_dump = true;
	}
	mfrc522.PICC_HaltA(); // Halt PICC
	mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
	prg_modus= PRG_MODE_NONE;
}

void loop() 
{
	if (Serial.available())
	{
		char ch = (char)Serial.read();
		if (ch == 'A' )
		{
			char strbuffer[4];
			Serial.setTimeout(20000L) ;
			Serial.println("READ MODUS ON");
			Serial.println("Please enter :Use dynamic Key B (Y/N) ?:(ending with #)");
			Serial.readBytesUntil('#', (char *) strbuffer, 3) ; // read family name from serial
			if ( strbuffer[0] == 'Y' || strbuffer[0] == 'y' )
				selected_useKeyB = true;
			else
				selected_useKeyB = false;
			Serial.print("Use Key B =" );Serial.println(selected_useKeyB);
			
			prg_modus = PRG_MODE_READ;
		}
		else if (ch == 'B' )
		{
			Serial.println("Write mode ON");
			if ( have_dump )
				prg_modus = PTG_MODE_WRITE;
			else
			{
				Serial.println("Write mode ON-FAIL dump not ready");
				prg_modus= PRG_MODE_NONE;
			}
		}
		else if ( ch == 'C' )
		{
			prg_modus = PRG_MODE_NONE; 
			Serial.println("DUMP OUT");
			Serial.println("---------------------------------------------------------");
			if ( have_dump )
			{
				for ( byte sector = 0 ; sector < 16; sector++ )
				{
					for ( int block = 0; block < 4; block++ )
					{
						for ( int index = 0; index < 16; index++ )
						{

							Serial.print(mydumpdata[sector][block][index] < 0x10 ? " 0" : " ");
							Serial.print(mydumpdata[sector][block][index], HEX);
							if ((index % 4) == 3) Serial.print(" ");
						}
						Serial.println(" ");					
					}
				}
			}
		}
		else if ( ch =='D' )
		{
			Serial.println("READ MODUS BLANK ON");
			prg_modus = PRG_MODE_READ_BLANK;
		}
		else if ( ch == 'E')
		{
			char strbuffer[4];
			memset(strbuffer,0,sizeof(strbuffer));
			Serial.setTimeout(20000L) ;
			Serial.println("READ MODUS:Please enter sector Nr:( ending with #)");
			Serial.readBytesUntil('#', (char *) strbuffer, 3) ; // read family name from serial
			selected_sector=atoi(strbuffer);
			Serial.print("Sector: "),Serial.println(selected_sector);
			Serial.println("READ MODUS:Please enter block Nr:( ending with #)");
			memset(strbuffer,0,sizeof(strbuffer));
			Serial.readBytesUntil('#', (char *) strbuffer, 3) ; // read family name from serial
			selected_block=atoi(strbuffer);
			Serial.print("Block: "),Serial.println(selected_block);
			
			Serial.println("READ MODUS:Use Key B (Y/N):( ending with #)");
			Serial.readBytesUntil('#', (char *) strbuffer, 3) ; // read family name from serial
			if ( strbuffer[0] == 'Y' )
				selected_useKeyB = true;
			else
				selected_useKeyB = false;
			Serial.print("Use Key B =" );Serial.println(selected_useKeyB);
			Serial.println("Please insert card in reader...");
			prg_modus = PRG_MODE_READ_SPECIFIC;
		}
		else if ( ch == 'F' )
		{
			CalculateKeyBDynamisch(&mfrc522.uid);
			//CalculateKeyB(&mfrc522.uid);
		}
  }

	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) 
	{
		//Serial.println("Not present");	
		return;
	}
	digitalWrite(led, HIGH);
	
	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial())
	{
		LastReadUid.uidByte[0] = 0;
		delay(10);
		return;
	}

	
	
	
	
	//CreateKeyA_KeyB(&mfrc522.uid);
	
	if ( prg_modus == PRG_MODE_READ )
	{
		//if ( selected_useKeyB ) 
		//	CalculateKeyBDynamisch(&mfrc522.uid);
		ReadCardwithKeys(selected_useKeyB );
		LastReadUid.uidByte[0] = 0;
	}
	else if ( prg_modus == PRG_MODE_READ_BLANK )
	{
		CheckCardIsEmpty(true);
		LastReadUid.uidByte[0] = 0;
	}
	else if ( prg_modus == PTG_MODE_WRITE  && have_dump )
	{
		Write2EmptyCard();
		LastReadUid.uidByte[0] = 0;
	}
	else if ( prg_modus == PRG_MODE_READ_SPECIFIC )
	{
		ReadCardwithKeysSpecified(selected_sector,selected_sector,selected_block,selected_block,selected_useKeyB);
		LastReadUid.uidByte[0] = 0;
	}
	
	digitalWrite(led, LOW);
	
	if ( memcmp(&LastReadUid, &mfrc522.uid,sizeof(LastReadUid)) != 0 )
	{
		
		Serial.print("Card UID:");    //Dump UID
		for (byte i = 0; i < mfrc522.uid.size; i++)
		{
			Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
			Serial.print(mfrc522.uid.uidByte[i], HEX);
		}
		Serial.print(" PICC type: ");   // Dump PICC type
		byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
		Serial.println(mfrc522.PICC_GetTypeName(piccType));
		memcpy(&LastReadUid,&mfrc522.uid,sizeof(LastReadUid));
		PrintSerialMenuText();
	}
	delay(10);
}







