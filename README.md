# DSPs-II---Tarea-8

Tarea 8: Reloj Alarma
Se realiza un reloj con alarma de la siguiente manera:

Se inicializa el reloj, con una alarma a cierta hora, minuto y segundo. Llamese alarm.hour, alarm.minute, alarm.second
Se declara una tarea seconds_task, la cual:
Se ejecuta de manera periódica cada segundo.
Cada ciclo incrementa una variable local "seconds" la cual al llegar a 60, libera un semáforo llamado "minutes_sempahore" y se regresa "seconds" a 0. 
Cuando el valor de "seconds" es igual a alarm.second, se hace "Set" de una bandera en un Event Group, en caso contrario se hace "Clear" de esta bandera.
Envia un mensaje a un mailbox llamado "time_queue", usando la estructura definida mas adelante, que contiene la variable seconds, y el identificador de segundos. Este mensaje se pasa por referencia.
Se declara una tarea minutes_task, la cual:
Espera a que se libere el semáforo "minutes_sempahore".
Una vez liberado el semáforo "minutes_sempahore", incrementa una variable local "minutes", para intentar tomar el semáforo "minutes_sempahore" una vez mas (en un ciclo infinito).
Cuando la variable "minutes" llega a 60, libera un semáforo llamado "hours_sempahore" y se regresa "minutes" a 0.
Cuando el valor de "minutes" es igual a alarm.minute, se hace "Set" de una bandera en un Event Group, en caso contrario se hace "Clear"  de esta bandera.
Envia un mensaje a un mailbox llamado "time_queue", usando la estructura definida mas adelante, que contiene la variable minutes, y el identificador de minutes. Este mensaje se pasa por referencia.
Se declara una tarea hours_task, la cual:
Espera a que se libere el semáforo "hours_sempahore".
Una vez liberado el semáforo "hours_sempahore", incrementa una variable local "hours", para intentar tomar el semáforo "hours_sempahore" una vez mas (en un ciclo infinito).
Cuando el valor de "hours" es igual a alarm.hour, se hace "Set" de una bandera en un Event Group, en caso contrario se hace "Clear" de esta bandera.
Cuando "hours" llega a 24 se regresa a 0.
Envia un mensaje a un mailbox llamado "time_queue", usando la estructura definida mas adelante, que contiene la variable hours, y el identificador de horas. Este mensaje se pasa por referencia.
Se declara una tarea alarm_task, la cual:
Espera a que las tres banderas del event group, accionados por las tareas de horas, minutos y segundos, esten en "Set", las tres al mismo tiempo. 
En caso de que las tres banderas esten accionadas al mismo tiempo, escribe "ALARM" en la terminal UART.
Posteriormente vuelve a esperar por el event group.
Se declara una tarea print_task, la cual:
Espera por mensajes en el mailbox "time_queue", y cada que llegan nuevos mensajes, actualiza el valor del tiempo local en horas y minutos y segundos, dependiendo de que tipo de mensaje se recibio.
Cada que llega un mensaje nuevo, imprime el valor de la hora actualizada en la terminal UART.
El puerto UART debe estar protegido correctamente, esto es: Los mensajes que se impriman en el no deben verse interrumpidos.
La estructura para los mensajes en el mailbox es la siguiente:
        typedef enum{seconds_type, minutes_type, hours_type} time_types_t;

        typedef struct

        {

            time_types_t time_type;   

            uint8_t value;

        }   time_msg_t;

Un diagrama de las tareas y su relación se muestra adjunto.
Esta tarea es por equipos.
