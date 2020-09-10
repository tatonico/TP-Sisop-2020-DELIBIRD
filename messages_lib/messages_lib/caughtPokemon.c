/*
 * caughtPokemon.c
 *
 *  Created on: 31 may. 2020
 *      Author: utnso
 */

#include "messages_lib.h"

mensajeCaught* llenarCaught( uint32_t resultadoCaught){
	mensajeCaught* msg = malloc(sizeof(mensajeCaught));
	msg->resultadoCaught=resultadoCaught;
	return msg;

}

void* serializarCaught (mensajeCaught* mensaje){
	void* stream    = malloc(sizeof(uint32_t));
	uint32_t offset = 0;

	memcpy(stream+offset, &(mensaje->resultadoCaught), sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	return stream;
}

mensajeCaught* deserializarCaught(void* streamRecibido){
	mensajeCaught* mensaje = malloc(sizeof(mensajeCaught));
	uint32_t offset        = 0;

	memcpy(&(mensaje->resultadoCaught), streamRecibido+offset, sizeof(uint32_t));

	return mensaje;
}

void destruirCaught(mensajeCaught* msg){
	free(msg);
}
