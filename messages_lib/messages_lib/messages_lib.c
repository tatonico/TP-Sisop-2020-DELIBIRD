/*
 * messages_lib.c
 *
 *  Created on: 12 abr. 2020
 *      Author: utnso
 */

#include "messages_lib.h"

uint32_t crearSocketCliente (char* ip, uint32_t puerto){
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family      = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(ip);
	direccionServidor.sin_port        = htons(puerto);

	uint32_t cliente=socket(AF_INET,SOCK_STREAM,0);
	if(connect(cliente,(void*) &direccionServidor,sizeof(direccionServidor)) != 0){

		return -1;
	}
	return cliente;
}

uint32_t sizeArgumentos (uint32_t colaMensaje, char* nombrePokemon, uint32_t cantidadPokemon){
	uint32_t size;
	switch(colaMensaje){
	case NEW_POKEMON:
		size = strlen(nombrePokemon) +  sizeof(uint32_t)*4;
		break;

	case APPEARED_POKEMON:
		size = strlen(nombrePokemon) + sizeof(uint32_t)*3;
		break;

	case CATCH_POKEMON:
		size = strlen(nombrePokemon) + sizeof(uint32_t)*3;
		break;

	case CAUGHT_POKEMON:
		size = sizeof(uint32_t);
		break;

	case GET_POKEMON: ;

		size = strlen(nombrePokemon) +  sizeof(uint32_t);
		break;

	case SUSCRIPCION_TIEMPO:
		size=3*sizeof(uint32_t);  break;

	case SUSCRIPCION:
		size=2*sizeof(uint32_t);break;

	case LOCALIZED_POKEMON:
		size=2*sizeof(uint32_t)+strlen(nombrePokemon)+cantidadPokemon*2*sizeof(uint32_t); break;

	default:
		printf("Error: el caso ingresado no esta contemplado\n");
		break;
	}
	return size;
}

int32_t enviarACK(uint32_t puerto,char* ip,  uint32_t modulo, uint32_t id, uint32_t idProceso){
	uint32_t idAEnviar=idProceso;



	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family      = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(ip);
	direccionServidor.sin_port        = htons(puerto);

	uint32_t cliente=socket(AF_INET,SOCK_STREAM,0);

	if(connect(cliente,(void*) &direccionServidor,sizeof(direccionServidor))<0){
			return -1;
		}

	void* stream= malloc(sizeof(uint32_t));
	memcpy(stream, &idAEnviar, sizeof(uint32_t));

	paquete* paqueteACK = llenarPaquete(modulo, ACK, sizeof(uint32_t), stream);

	insertarIdCorrelativoPaquete(paqueteACK, id);
	void* paqueteACKSerializado = serializarPaquete(paqueteACK);

	uint32_t i= send(cliente, paqueteACKSerializado, sizePaquete(paqueteACK), 0);

	destruirPaquete(paqueteACK);
	free(paqueteACKSerializado);
	return i;
}

uint32_t obtenerIdProcesoDeAck(void* stream){
	uint32_t id;
	memcpy(&id, stream, sizeof(uint32_t));
	return id;
}

void liberarArrayBidimensionalChar(char** array){
	for(uint32_t i=0; *(array + i)!=NULL;i++){
		free(*(array+i));

	}
	free(array);
}

