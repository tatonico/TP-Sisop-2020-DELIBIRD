/*
 * gamecard.c
 *
 *  Created on: 12 may. 2020
 *      Author: juancito
 */



#include "gamecard.h"

int main(int argc , char* argv[]) {
	pathConfigGC=argv[1];
	sem_t * semaforoFinalizacionGC=malloc(sizeof(sem_t));
	sem_init(semaforoFinalizacionGC,0,0);

	t_config* configGamecard;
	configGamecard = crearYleerConfig();
	gamecardLogger = log_create(pathLoggerPrincipal,"gamecard",false,LOG_LEVEL_INFO);
	//iniciar_logger(pathLoggerPrincipal, "GAMECARD"); //no se que culo le pasa q no lo crea
	gamecardLogger2=log_create("gamecardLoggerSecundario.log","gamecard", false, LOG_LEVEL_INFO);


	log_info(gamecardLogger, "Inicio de programa: Gamecard.");
	listaArchivos=inicializarListaMutex();
	mutexPrueba=malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutexPrueba,NULL);

	crearDirectorio("Files","/home/utnso/tp-2020-1c-Programadores-en-Fuga/Gamecard/TALL_GRASS/", DIRECTORIO);
	crearDirectorio("Blocks","/home/utnso/tp-2020-1c-Programadores-en-Fuga/Gamecard/TALL_GRASS/", DIRECTORIO);

	iniciarMetadata();
	iniciarBitmap();
	inicializarListaBloques();


	suscribirseColasBrokerGC();

	crearHiloServidorGameboyGC(&hiloServidorDeEscucha);

	sem_wait(semaforoFinalizacionGC);

	liberarPrograma(configGamecard,gamecardLogger);


	return EXIT_SUCCESS;
}


t_config* crearYleerConfig(){
	t_config * configGamecard = config_create(pathConfigGC);
	puertoBrokerGC = config_get_int_value(configGamecard, "PUERTO_BROKER");//5002;
	ipBrokerGC = config_get_string_value(configGamecard, "IP_BROKER");//"127.0.0.1"; //
	tiempoRetardoGC = config_get_int_value(configGamecard, "TIEMPO_DE_RETARDO_OPERACION"); //
	tiempoReintentoOperacion=config_get_int_value(configGamecard, "TIEMPO_DE_REINTENTO_OPERACION"); //
	puntoMontaje = config_get_string_value(configGamecard, "PUNTO_MONTAJE_TALLGRASS");
	tiempoReconexionGC = config_get_int_value(configGamecard, "TIEMPO_DE_REINTENTO_CONEXION");
	pathLoggerPrincipal=config_get_string_value(configGamecard, "LOG_FILE");
	idProcesoGamecard = config_get_int_value(configGamecard, "ID_PROCESO");
	puertoGamecardGC=config_get_int_value(configGamecard, "PUERTO_GAMECARD");
	return configGamecard;
}


//int crearHiloConexionBroker(void* config, pthread_t* hilo){
//	uint32_t err=pthread_create(hilo,NULL,suscribirseColasBroker,(void*)config);
//				if(err!=0){
//					log_info(gamecardLogger2,"Hubo un problema en la creación del hilo para conectarse al broker");
//					return err;
//				}
//
//	pthread_detach(*hilo);
//	return 0;
//}


void suscribirseColasBrokerGC() {

	mensajeSuscripcion* mensajeSuscripcionNew=llenarSuscripcion(NEW_POKEMON,idProcesoGamecard);
	mensajeSuscripcion * mensajeSuscripcionCatch=llenarSuscripcion(CATCH_POKEMON,idProcesoGamecard);
	mensajeSuscripcion* mensajeSuscripcionGet=llenarSuscripcion(GET_POKEMON,idProcesoGamecard);


	pthread_create(&threadSuscripcionNew, NULL, suscribirseColaGC,(void*) (mensajeSuscripcionNew));
	pthread_detach(threadSuscripcionNew);


	pthread_create(&threadSuscripcionCatch, NULL, suscribirseColaGC,(void*) (mensajeSuscripcionCatch));
	pthread_detach(threadSuscripcionCatch);


	pthread_create(&threadSuscripcionGet, NULL, suscribirseColaGC,(void*) (mensajeSuscripcionGet));
	pthread_detach(threadSuscripcionGet);




}



void* suscribirseColaGC(void* msgSuscripcion) {
	mensajeSuscripcion* msg = (mensajeSuscripcion*) msgSuscripcion;
	//uint32_t sizeStream = sizeof(uint32_t);
	//void* streamMsgSuscripcion = serializarSuscripcion(msg);
	//destruirSuscripcion(msg);
	log_info(gamecardLogger2,"Voy a llenar el paquete");
	//paquete* paq = llenarPaquete(GAMECARD, SUSCRIPCION, sizeStream,streamMsgSuscripcion);

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(ipBrokerGC);
	direccionServidor.sin_port = htons(puertoBrokerGC);

	uint32_t cliente = socket(AF_INET, SOCK_STREAM, 0);
	log_info(gamecardLogger2,"cliente: %d", cliente);


	if(connect(cliente, (void*) &direccionServidor,sizeof(direccionServidor)) <0){
		cliente=reconectarseAlBrokerGC();
	}
	cliente=enviarSuscripcion(cliente, msgSuscripcion);

	while(1){
		paquete* paqueteRespuesta=recibirPaquete(cliente);

		while(paqueteRespuesta==NULL){
			cliente=reconectarseAlBrokerGC();
			cliente=enviarSuscripcion(cliente, msg);
			paqueteRespuesta=recibirPaquete(cliente);
		}

		loggearMensaje(paqueteRespuesta, gamecardLogger);


		int32_t resultadoAck=enviarACK(puertoBrokerGC,ipBrokerGC,  GAMECARD, paqueteRespuesta->id, idProcesoGamecard);
				while(resultadoAck<0){
					cliente=reconectarseAlBrokerGC();
					cliente=enviarSuscripcion(cliente, msg);
					resultadoAck=enviarACK(puertoBrokerGC,ipBrokerGC,  GAMECARD, paqueteRespuesta->id, idProcesoGamecard);
				}

		switch(paqueteRespuesta->tipoMensaje){
			case NEW_POKEMON:;
				pthread_t threadAppeared;
				pthread_create(&threadAppeared, NULL, atenderNew,(void*) (paqueteRespuesta));
				pthread_detach(threadAppeared);
				break;
			case GET_POKEMON:;
				pthread_t threadLocalized;
				pthread_create(&threadLocalized, NULL, atenderGet,(void*) (paqueteRespuesta));
				pthread_detach(threadLocalized);//recordar destruir el paquete
				break;
			case CATCH_POKEMON:;
				pthread_t threadCaught;
				pthread_create(&threadCaught, NULL, atenderCatch, (void*) (paqueteRespuesta));
				pthread_detach(threadCaught);
				break;
			default: break; //esto no puede pasar
			}


			}

	return NULL;
}

//uint32_t reconectarseAlBroker(uint32_t cliente,void* direccionServidor,socklen_t length){
//	log_info(gamecardLogger, "Conexión fallida con el Broker");
//	log_info(gamecardLogger, "Reintentando conexión en %i segundos...",tiempoReconexionGC);
//	sleep(tiempoReconexionGC);
//	while(connect(cliente, direccionServidor,length)<0){
//		log_info(gamecardLogger,"El reintento de conexión no fue exitoso");
//		log_info(gamecardLogger, "Reintentando conexión en %i segundos...",tiempoReconexionGC);
//		sleep(tiempoReconexionGC);
//
//
//	}
//	//log_info(teamLogger, "El reintento de conexión fue exitoso\n");
//	return 0;
//}

uint32_t enviarSuscripcion(uint32_t socket, mensajeSuscripcion* msg){
	uint32_t cliente=socket;

	void* streamMsgSuscripcion=serializarSuscripcion(msg);

	paquete* paq=llenarPaquete(GAMECARD,SUSCRIPCION,sizeArgumentos(SUSCRIPCION, "",0), streamMsgSuscripcion);

	uint32_t bytes = sizePaquete(paq);

	void* stream   = serializarPaquete(paq);

	while(send(cliente,stream,bytes,0)<0){
		cliente=reconectarseAlBrokerGC();
	}

	uint32_t respuesta = -1;

	recv(cliente,&respuesta,sizeof(uint32_t),0);
	//printf("Socket: %i, cola: %i\n", cliente, msg->cola);

	if(respuesta!=CORRECTO){
		log_info(gamecardLogger, "Hubo un problema con la suscripción a una cola.");
	}

	destruirPaquete(paq);
	free(stream);
	return cliente;
}

uint32_t reconectarseAlBrokerGC(){
	log_info(gamecardLogger, "Conexión fallida con el Broker\n");
	log_info(gamecardLogger2, "Conexión fallida con el Broker\n");
	log_info(gamecardLogger, "Reintentando conexión en %i segundos...\n",tiempoReconexionGC);
	sleep(tiempoReconexionGC);
	uint32_t cliente;
	struct sockaddr_in direccionServidor;
	uint32_t i=0;
	do{


		direccionServidor.sin_family      = AF_INET;
		direccionServidor.sin_addr.s_addr = inet_addr(ipBrokerGC);
		direccionServidor.sin_port        = htons(puertoBrokerGC);

		cliente=socket(AF_INET,SOCK_STREAM,0);
		if(i>0){
			log_info(gamecardLogger,"El reintento de conexión no fue exitoso\n");
			log_info(gamecardLogger2,"El reintento de conexión no fue exitoso\n");
		}
		i++;
		log_info(gamecardLogger, "Reintentando conexión en %i segundos...\n",tiempoReconexionGC);
		sleep(tiempoReconexionGC);

	}while(connect(cliente,(void*) &direccionServidor,sizeof(direccionServidor))<0);

	log_info(gamecardLogger, "El reintento de conexión fue exitoso\n");
	log_info(gamecardLogger2, "El reintento de conexión fue exitoso\n");

	return cliente;
}



void crearHiloServidorGameboyGC(pthread_t* hilo) {
	pthread_create(hilo, NULL, iniciarServidorGameboyGC, NULL);

	pthread_detach(*hilo);


}

void* iniciarServidorGameboyGC(void* arg) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(puertoGamecardGC);
	uint32_t servidor = socket(AF_INET, SOCK_STREAM, 0);

	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("Falló el bind");

	} else {
		log_info(gamecardLogger,"Estoy escuchando");
		while (1)  						//para recibir n cantidad de conexiones
			esperar_cliente(servidor);
	}
	return NULL;
}

void esperar_cliente(uint32_t servidor) {

	listen(servidor, 100);
	struct sockaddr_in dir_cliente;

	uint32_t tam_direccion = sizeof(struct sockaddr_in);
	log_info(gamecardLogger,"Espero un nuevo cliente");
	uint32_t* socket_cliente = malloc(sizeof(uint32_t));
	*socket_cliente = accept(servidor, (void*) &dir_cliente, &tam_direccion);
	log_info(gamecardLogger,"Gestiono un nuevo cliente");
	pthread_t threadAtencionGameboyGC;
	pthread_create(&threadAtencionGameboyGC, NULL, atenderCliente,(void*) (socket_cliente));
	pthread_detach(threadAtencionGameboyGC);
}

void* atenderCliente(void* sock) {
//	log_info(gamecardLogger2,"atiendo cliente");
	uint32_t* socket = (uint32_t*) sock;
//	log_info(gamecardLogger2,"hola llegue");
	paquete* paquete = recibirPaquete(*socket);
	uint32_t respuesta = 0;
	if (paquete == NULL) {
		respuesta = INCORRECTO;
	} else {
		respuesta = CORRECTO;
	}

	send(*socket, (void*) (&respuesta), sizeof(uint32_t), 0);
	free(socket);

	//log_info(gamecardLogger2,"hice el send: %i", respuesta);
	//log_info(gamecardLogger2,"recibi: %i", paquete->sizeStream);
	loggearMensaje(paquete,gamecardLogger);
	switch (paquete->tipoMensaje) {
	case NEW_POKEMON:;

		atenderNew((void*)paquete);

		break;
	case GET_POKEMON:;

		atenderGet((void*)paquete);

		break;
	case CATCH_POKEMON:;
		atenderCatch((void*)paquete);
		break;
	default:
		log_info(gamecardLogger2,"leyo cualquiera");
		break;
	}

	return NULL;
}

void* atenderNew(void* paq) {
	log_info(gamecardLogger,"Inicia proceso de atencion al New");
	paquete* paqueteNew = (paquete*) paq;
	uint32_t idNew = paqueteNew->id;
	mensajeNew* msgNew = deserializarNew(paqueteNew->stream);
	//log_info(gamecardLogger2,"deserializado");

	pokemonEnPosicion* pokeEnPosicion = malloc(sizeof(pokemonEnPosicion));
	pokeEnPosicion->pokemon=malloc(strlen(msgNew->pokemon)+1);
	strcpy(pokeEnPosicion->pokemon,msgNew->pokemon);
//	pokeEnPosicion->pokemon = msgNew->pokemon;
	pokeEnPosicion->cantidad = msgNew->cantidad;
	pokeEnPosicion->id = idNew;
	(pokeEnPosicion->posicion).x = msgNew->posX;
	(pokeEnPosicion->posicion).y = msgNew->posY;

	//free(msg);
	//To do :
	//log_info(gamecardLogger2,"Atiendo new del pokemon: %s. Posicion: (%i, %i).", msgNew->pokemon, msgNew->posX, msgNew->posY);
	archivoHeader* archivoPoke= obtenerArchivoPokemon(pokeEnPosicion->pokemon);


	FILE* archivoMetadata=abrirArchivo(archivoPoke);


	//log_info(gamecardLogger2, "comienzo a obtener lista de %s", msgNew->pokemon);
	t_list* listaPosCantidad=obtenerListaPosicionCantidadDeArchivo(archivoPoke);

	//log_info(gamecardLogger2, "termino de obtener lista de %s", msgNew->pokemon);
	posicionCantidad* encontrado= buscarPosicionCantidad(listaPosCantidad, pokeEnPosicion->posicion);


	if(encontrado!=NULL){
		(encontrado->cantidad)+=pokeEnPosicion->cantidad;
	}else{
		posicionCantidad* posAgregar=malloc(sizeof(posicionCantidad));
		posAgregar->cantidad=pokeEnPosicion->cantidad;
		(posAgregar->posicion).x=(pokeEnPosicion->posicion).x;
		(posAgregar->posicion).y=(pokeEnPosicion->posicion).y;
		list_add(listaPosCantidad, (void*) posAgregar);
	}

	//log_info(gamecardLogger2, "comienzo a actualizar posiciones de %s", msgNew->pokemon);
	actualizarPosicionesArchivo(archivoPoke,listaPosCantidad);
	//log_info(gamecardLogger2, "termino de actualizar posiciones de %s", msgNew->pokemon);
	list_destroy_and_destroy_elements(listaPosCantidad, free);

	sleep(tiempoRetardoGC);
	cerrarArchivo(archivoPoke,archivoMetadata);

	log_info(gamecardLogger2,"Cierro el archivo del pokemon: %s. Posicion: (%i, %i).", msgNew->pokemon, msgNew->posX, msgNew->posY);
	log_info(gamecardLogger,"Cierro el archivo del pokemon: %s. Posicion: (%i, %i).", msgNew->pokemon, msgNew->posX, msgNew->posY);
	enviarAppeared(pokeEnPosicion);
	free(pokeEnPosicion->pokemon);
	free(pokeEnPosicion);
	destruirNew(msgNew);
	destruirPaquete(paq);
	return NULL;

}

void enviarAppeared(pokemonEnPosicion* pokeEnPosicion) {
	log_info(gamecardLogger,"Inicia proceso envio Appeared");
	uint32_t cliente = crearSocketCliente(ipBrokerGC, puertoBrokerGC);
	mensajeAppeared* msgAppeared = llenarAppeared(pokeEnPosicion->pokemon,(pokeEnPosicion->posicion).x,(pokeEnPosicion->posicion).y);
	void* streamMsg = serializarAppeared(msgAppeared);
	paquete* paq = llenarPaquete(GAMECARD, APPEARED_POKEMON,sizeArgumentos(APPEARED_POKEMON, msgAppeared->pokemon, 0),streamMsg);
	insertarIdCorrelativoPaquete(paq, (pokeEnPosicion->id));
	void* paqueteSerializado = serializarPaquete(paq);
	destruirAppeared(msgAppeared);
	//destruirPaquete(paq);

	send(cliente, paqueteSerializado, sizePaquete(paq), 0);
	free(paqueteSerializado);
	destruirPaquete(paq);

}

void* atenderGet(void* paq) {
	log_info(gamecardLogger,"Inicia proceso de atencion al Get");
	paquete* paqueteGet = (paquete*) paq;
	mensajeGet* msgGet = deserializarGet(paqueteGet->stream);
	uint32_t idGet = paqueteGet->id;
	log_info(gamecardLogger2,"deserializado");
	destruirPaquete(paqueteGet);

	pokemonADevolver* pokeADevolver = malloc(sizeof(pokemonADevolver));
	pokeADevolver->pokemon=malloc(strlen(msgGet->pokemon)+1);
	strcpy(pokeADevolver->pokemon, msgGet->pokemon);
	//pokeADevolver->pokemon = msgGet->pokemon;
	pokeADevolver->id = idGet;
	log_info(gamecardLogger2,"Atiendo Get del pokemon: %s", msgGet->pokemon);

	char* pathArchivoExiste=string_from_format("%s%s",pathFiles,pokeADevolver->pokemon);
	if(archivoExiste(pathArchivoExiste)){
		log_info(gamecardLogger2,"EXISTE");
		archivoHeader* archivoPoke = obtenerArchivoPokemon(pokeADevolver->pokemon);
		FILE* archivoMetadata = abrirArchivo(archivoPoke);
		t_list* listaPosCantidad=obtenerListaPosicionCantidadDeArchivo(archivoPoke);

		uint32_t cantPosiciones = list_size(listaPosCantidad);
		log_info(gamecardLogger2, "CANTIDAD POSICIONES %i", cantPosiciones);

		pokeADevolver->cantPosiciones = cantPosiciones;
		pokeADevolver->posiciones = conseguirPosicionesCantidad(listaPosCantidad);

		//list_destroy_and_destroy_elements(listaPosCantidad,free);
		sleep(tiempoRetardoGC);
		cerrarArchivo(archivoPoke,archivoMetadata);
	}else{
		log_info(gamecardLogger2,"NO EXISTE");
		pokeADevolver->posiciones = NULL;//revisar este caso luego
		pokeADevolver->cantPosiciones=0;
	}

	log_info(gamecardLogger2,"Pokemon:%s",pokeADevolver->pokemon);
	log_info(gamecardLogger2,"Cantidad de Posiciones:%i",pokeADevolver->cantPosiciones);

	for(uint32_t i=0;i<pokeADevolver->cantPosiciones;i++){
		posicion aux = *(pokeADevolver->posiciones+i);
		log_info(gamecardLogger2,"Posicion: %i-%i",aux.x,aux.y);
	}

	enviarLocalized(pokeADevolver);
	destruirGet(msgGet);
	free(pathArchivoExiste);
	free(pokeADevolver->pokemon);
	free(pokeADevolver);
	//destruirPaquete(paq);
	return NULL;
}

void enviarLocalized(pokemonADevolver* pokeADevolver) {
	log_info(gamecardLogger,"Inicia proceso envio Localized");
	uint32_t cliente = crearSocketCliente(ipBrokerGC, puertoBrokerGC);
	//mensajeLocalized* msgLocalized = malloc(sizeof(mensajeLocalized));
	mensajeLocalized* msgLocalized = llenarLocalized(pokeADevolver->pokemon,pokeADevolver->cantPosiciones,pokeADevolver->posiciones);



	//msgLocalized->pokemon = pokeADevolver->pokemon;
	//msgLocalized->cantidad = pokeADevolver->cantPosiciones;
	//msgLocalized->arrayPosiciones = pokeADevolver->posicion;
	//msgLocalized->sizePokemon = strlen(msgLocalized->pokemon) + 1;
	void* streamMsg = serializarLocalized(msgLocalized);

//	mensajeLocalized* msg2=deserializarLocalized(streamMsg);
//	log_info(gamecardLogger2,"-------------------------------------");
//	for(uint32_t i=0;i<msg2->cantidad;i++){
//				posicion aux = *(msg2->arrayPosiciones + i);
//				log_info(gamecardLogger2,"Posicion: %i-%i",aux.x,aux.y);
//			}
	paquete* paq = llenarPaquete(GAMECARD, LOCALIZED_POKEMON,sizeArgumentos(LOCALIZED_POKEMON, msgLocalized->pokemon, msgLocalized->cantidad),streamMsg);
	insertarIdCorrelativoPaquete(paq, (pokeADevolver->id));
	void* paqueteSerializado = serializarPaquete(paq);
	destruirLocalized(msgLocalized);
	//destruirPaquete(paq);
	loggearMensaje(paq, gamecardLogger2);
	send(cliente, paqueteSerializado, sizePaquete(paq), 0);
	free(paqueteSerializado);
}

void* atenderCatch(void* paq) {
	log_info(gamecardLogger,"Inicia proceso de atencion al Catch");
	paquete* paqueteCatch = (paquete*) paq;
	uint32_t idCatch = paqueteCatch->id;
	mensajeCatch* msgCatch = deserializarCatch(paqueteCatch->stream);
	log_info(gamecardLogger2,"deserializado");
	pokemonAAtrapar* pokeAAtrapar = malloc(sizeof(pokemonAAtrapar));
	pokeAAtrapar->id = idCatch;
	pokeAAtrapar->pokemon = msgCatch->pokemon;
	(pokeAAtrapar->posicion).x = msgCatch->posX;
	(pokeAAtrapar->posicion).y = msgCatch->posY;
	log_info(gamecardLogger2,"Pokemon:%s, posicion:%i-%i",pokeAAtrapar->pokemon,(pokeAAtrapar->posicion).x,(pokeAAtrapar->posicion).y);

	archivoHeader* archivoPokeCatch= obtenerArchivoPokemon(pokeAAtrapar->pokemon);
	FILE* archivoMetadata=abrirArchivo(archivoPokeCatch);
	t_list* listaPosCantidadCatch=obtenerListaPosicionCantidadDeArchivo(archivoPokeCatch);
	posicionCantidad* encontrado= buscarPosicionCantidad(listaPosCantidadCatch, pokeAAtrapar->posicion);
	log_info(gamecardLogger2,"Size lista antes de comprobar:%i",list_size(listaPosCantidadCatch));
	if(encontrado == NULL){
		log_info(gamecardLogger2,"NULLACIO");
		pokeAAtrapar->resultado = FAIL;
	}else{
		uint32_t idPosicion = buscarIdCantidad(listaPosCantidadCatch, pokeAAtrapar->posicion);
		if(encontrado->cantidad>1){
			log_info(gamecardLogger2,"Reemplazo");

			encontrado->cantidad = encontrado->cantidad - 1;
			//list_replace(listaPosCantidadCatch, idPosicion, encontrado);
		}else{
			log_info(gamecardLogger2,"Elimino linea");
			list_remove(listaPosCantidadCatch, idPosicion);
			//list_remove_and_destroy_element(listaPosCantidadCatch, idPosicion, free);
		}
		actualizarPosicionesArchivo(archivoPokeCatch,listaPosCantidadCatch);
		pokeAAtrapar->resultado = OK;
	}
	log_info(gamecardLogger2,"Size lista despues de comprobar:%i",list_size(listaPosCantidadCatch));

	sleep(tiempoRetardoGC);
	cerrarArchivo(archivoPokeCatch,archivoMetadata);
	log_info(gamecardLogger,"Cierro el archivo del pokemon: %s. Posicion: (%i, %i).", msgCatch->pokemon, msgCatch->posX, msgCatch->posY);

	enviarCaught(pokeAAtrapar); //Momentaneo hasta saber bien que hacer con fileSystem
	//free(pokeAAtrapar->pokemon);
	//free(pokeAAtrapar);
	//destruirCatch(msgCatch);
	destruirPaquete(paq);
	return NULL;
}

void enviarCaught(pokemonAAtrapar* pokeAAtrapar) {
	log_info(gamecardLogger,"Inicia proceso envio Caught");
	uint32_t cliente = crearSocketCliente(ipBrokerGC, puertoBrokerGC);
	mensajeCaught* msgCaught = llenarCaught(pokeAAtrapar->resultado);

	void* streamMsg = serializarCaught(msgCaught);
	paquete* paq = llenarPaquete(GAMECARD, CAUGHT_POKEMON,
	sizeArgumentos(CAUGHT_POKEMON, NULL, BROKER), streamMsg);
	insertarIdCorrelativoPaquete(paq, (pokeAAtrapar->id));
	void* paqueteSerializado = serializarPaquete(paq);
	//free(msgLocalized);
	//destruirPaquete(paq);
	send(cliente, paqueteSerializado, sizePaquete(paq), 0);
	free(paqueteSerializado);

}



void liberarPrograma(t_config* configGamecard,t_log* gamecardLogger){
	log_destroy(gamecardLogger);
	config_destroy(configGamecard);
}



