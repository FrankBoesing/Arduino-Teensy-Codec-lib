
#include "debug.h"


void initDBG()
  {
  DBG.begin( DBG_BAUD );  
  #if (DBG==Serial2)//as soon as possible in setup()
	//Serial2.c changes pinconfiguration, switch back to audio-config: 
	CORE_PIN9_CONFIG = PORT_PCR_MUX( 6 ); 
	CORE_PIN10_CONFIG = PORT_PCR_MUX( 2 );
	//Switch Serial2 to pins 26,31:
	CORE_PIN26_CONFIG = PORT_PCR_PE | PORT_PCR_PS | PORT_PCR_MUX( 3 );
	CORE_PIN31_CONFIG = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX( 3 );
  #endif
  }