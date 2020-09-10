/*
 * appearedPokemonTeam.c
 *
 *  Created on: 31 may. 2020
 *      Author: utnso
 */

#include "messages_lib.h"

mensajeAppeared* llenarAppeared(char* pokemon, uint32_t posX, uint32_t posY){
	mensajeAppeared* msg = malloc(sizeof(mensajeAppeared));
	msg->sizePokemon=strlen(pokemon);
	msg->pokemon=malloc(msg->sizePokemon + 1);
	strcpy(msg->pokemon,pokemon);
	msg->posX=posX;
	msg->posY=posY;

	return msg;
}


//mensajeAppeared* llenarAppearedMemoria(char* pokemon, uint32_t posX, uint32_t posY){
//	mensajeAppeared* msg = malloc(sizeof(mensajeAppeared));
//	msg->sizePokemon=strlen(pokemon)+1;
//	msg->pokemon=malloc(msg->sizePokemon);
//	memcpy(msg->pokemon,pokemon,msg->sizePokemon);
//	msg->posX=posX;
//	msg->posY=posY;
//
//	return msg;
//}


void* serializarAppeared(mensajeAppeared* mensaje){
	void* stream    = malloc(sizeof(uint32_t)*3 + mensaje->sizePokemon);
	uint32_t offset = 0;

	memcpy(stream+offset, &(mensaje->sizePokemon), sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset, mensaje->pokemon, mensaje->sizePokemon);
	offset+=(mensaje->sizePokemon);
	memcpy(stream+offset, &(mensaje->posX), sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream+offset, &(mensaje->posY), sizeof(uint32_t));
	offset+= sizeof(uint32_t);

	return stream;
}

//void* serializarAppearedMemoria (mensajeAppeared* mensaje){
//	void* stream    = malloc(sizeof(uint32_t)*3 + mensaje->sizePokemon);
//	uint32_t offset = 0;
//
//	memcpy(stream+offset, &(mensaje->sizePokemon), sizeof(uint32_t));
//	offset+= sizeof(uint32_t);
//	memcpy(stream+offset, mensaje->pokemon, mensaje->sizePokemon);
//	offset+=(mensaje->sizePokemon);
//	memcpy(stream+offset, &(mensaje->posX), sizeof(uint32_t));
//	offset+= sizeof(uint32_t);
//	memcpy(stream+offset, &(mensaje->posY), sizeof(uint32_t));
//	offset+= sizeof(uint32_t);
//
//	return stream;
//}

mensajeAppeared* deserializarAppeared (void* streamRecibido){
	uint32_t offset              = 0;
	mensajeAppeared* mensaje = malloc(sizeof(mensajeAppeared));

	memcpy(&(mensaje->sizePokemon), streamRecibido+offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);

	mensaje->pokemon=malloc(mensaje->sizePokemon+1);
	memcpy(mensaje->pokemon, streamRecibido+offset, mensaje->sizePokemon);
	offset+=(mensaje->sizePokemon);
	*(mensaje->pokemon + mensaje->sizePokemon)='\0';

	memcpy(&(mensaje->posX), streamRecibido+offset, sizeof(uint32_t));
	offset+=sizeof(uint32_t);
	memcpy(&(mensaje->posY), streamRecibido+offset, sizeof(uint32_t));

	return mensaje;
}

void destruirAppeared(mensajeAppeared* msg){
	free(msg->pokemon);
	free(msg);
}
