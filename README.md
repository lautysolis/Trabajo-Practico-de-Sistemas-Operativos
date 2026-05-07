# Trabajo Práctico - Sistemas Operativos

Este proyecto consiste en el desarrollo de un simulador de sistema operativo distribuido realizado en lenguaje C para la materia Sistemas Operativos de la UTN FRBA. El objetivo principal del trabajo es aplicar conceptos fundamentales de sistemas operativos, concurrencia, comunicación entre procesos, manejo de memoria y sistemas de archivos mediante la implementación de distintos módulos que interactúan entre sí.

El sistema está compuesto por múltiples módulos independientes que se comunican mediante sockets y protocolos propios, simulando el funcionamiento de componentes reales de un sistema operativo distribuido. Entre ellos se encuentran procesos encargados de la coordinación general del sistema, módulos workers responsables de la ejecución de tareas y un módulo storage orientado a la administración y persistencia de datos.

Durante el desarrollo del proyecto se trabajó con:
- Programación concurrente utilizando threads y sincronización.
- Comunicación cliente-servidor mediante sockets.
- Serialización y deserialización de estructuras.
- Manejo de archivos y persistencia de información.
- Configuración dinámica mediante archivos `.config`.
- Logging y monitoreo de eventos del sistema.
- Gestión de memoria y liberación de recursos.
- Arquitectura modular y desacoplada.

El proyecto fue desarrollado íntegramente en C sobre entorno Linux, utilizando herramientas y bibliotecas provistas por la cátedra, siguiendo una arquitectura orientada a procesos distribuidos.

## Objetivos del proyecto

- Comprender el funcionamiento interno de un sistema operativo.
- Simular la interacción entre distintos componentes distribuidos.
- Aplicar conceptos de planificación, concurrencia y comunicación entre procesos.
- Trabajar colaborativamente utilizando Git y GitHub.
- Desarrollar habilidades de debugging, testing y resolución de problemas complejos.

## Tecnologías utilizadas

- Lenguaje C
- Linux
- Sockets
- Threads
- Semáforos y sincronización
- Makefiles
- Git/GitHub
- Commons Library (UTN SO)

## Estructura general

El repositorio se encuentra dividido en distintos módulos independientes, cada uno con responsabilidades específicas dentro del sistema. La comunicación entre módulos permite simular el comportamiento integral de un sistema operativo real.
