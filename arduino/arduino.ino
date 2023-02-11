

void setup(){
	pinMode(13,OUTPUT);
}

void loop(){
	delay(1); 
	unsigned int val=analogRead(A0);
	Serial.write((unsigned char*)&val,2);  
}
