# Taquilla Virtual
_Taquilla Virtual_ is a **Distributed Virtual Ticketing System** developed as part of the course "[Programación Concurrente y Distribuída](https://secretaria.uvigo.gal/docnet-nuevo/guia_docent/?centre=305&ensenyament=V05G301V01&assignatura=V05G301V01306&any_academic=2023_24)" in the Telecommunications Engineering Degree at the Universidad de Vigo (2023 - 2024).

## About The Project
This project implements a distributed ticketing system using a [token-based mutual exclusion algorithm](https://en.wikipedia.org/wiki/Ricart%E2%80%93Agrawala_algorithm) to coordinate ticket purchases, cancellations, reservations, and more across multiple distributed nodes. The system integrates key concepts of concurrency such as message passing, distributed synchronization, mutual exclusion, and priority handling.

The project features:
- Token-based mutual exclusion to manage concurrent operations.
- Dynamic distributed system with a variable number of nodes.
- Message-passing communication between nodes.
- Priority handling to avoid starvation in the system
- Optimized query execution allowing concurrent read operations.
- Support for different ticketing processes, including reservations, purchases, cancellations, and administrative actions.

## How To Run
### Compilation
Make sure you have the [GCC](https://gcc.gnu.org) and [Make](https://www.gnu.org/software/make/) tools installed on your system. Then compile the project with:
```bash
make -C src
```
This command will generate the executable files inside the `src/` directory.

### Execution
#### System
Once compiled, you can run the system with:
```bash
./src/ejecutar_nodos.sh <nodos> <procesos_pagos> <procesos_anulaciones> <procesos_reservas> <procesos_administracion> <procesos_consultas> <timeout>
```
| Option | Description | Example |
|--------|-------------|---------|
| `nodos` | Amount of nodes | `4` |
| `procesos_pagos` | Number of paying processes on each node | `2` |
| `procesos_anulaciones` | Number of cancelling processes on each node | `2` |
| `procesos_reservas` | Number of reservation processes on each node | `2` |
| `procesos_administracion` | Number of administrative processes on each node | `2` |
| `procesos_consultas` | Number of consulting processes on each node | `2` |
| `timeout` | Time to keep the system running in seconds | `60` |
##### Example
```bash
./src/ejecutar_nodos.sh 4 2 2 2 2 2 60
```
*The number of nodes is hardcoded in [`utils.h`](src/utils.h). If you want to use a different value, modify line 3 and recompile the project.*

#### Clients
Then you can simulate a client with:
```bash
./src/cliente <nodos>
```
| Option | Description | Example |
|--------|-------------|---------|
| `nodos` | Amount of nodes | `4` |
##### Example
```bash
./src/cliente 4
```

Or you can simulate a randomized client with:
```bash
./src/cliente_rand <nodos> <solicitudes> <tiempo_max_solicitudes> <solicitudes_pago> <solicitudes_anulacion> <solicitudes_reserva> <solicitudes_administracion> <solicitudes_consulta>
```
| Option | Description | Example |
|--------|-------------|---------|
| `nodos` | Amount of nodes | `4` |
| `solicitudes` | Amount of requests | `100` |
| `tiempo_max_solicitudes` | Maximum time between requests in milliseconds | `500` |
| `solicitudes_pago` | Enable (`1`) or disable (`0`) paying requests | `1` |
| `solicitudes_anulacion` | Enable (`1`) or disable (`0`) cancelling requests | `0` |
| `solicitudes_reserva` | Enable (`1`) or disable (`0`) reservation requests | `0` |
| `solicitudes_administracion` | Enable (`1`) or disable (`0`) management requests | `1` |
| `solicitudes_consulta` | Enable (`1`) or disable (`0`) consulting requests | `1` |
##### Example
```bash
./src/cliente_rand 4 100 500 1 0 0 1 1
```

#### Termination
After every execution, it is recommended to terminate all nodes with:
```bash
./src/kill <nodos>
```
| Option | Description | Example |
|--------|-------------|---------|
| `nodos` | Amount of nodes | `4` |
##### Example
```bash
./src/kill 4
```

## About The Code
Refer to [`Presentación - Diseño Avanzado`](docs/Presentación%20-%20Diseño%20Avanzado.pdf) and [`Presentación - Diseño Final`](docs/Presentación%20-%20Diseño%20Final.pdf) for a high level overview of the project.

Refer to [`Especificaciones.pdf`](docs/Especificaciones.pdf), [`Documentación.pdf`](docs/Documentación.pdf), [`Requisitos.pdf`](docs/Requisitos.pdf), and [`Pruebas.pdf`](docs/Pruebas.pdf) for an in-depth explanation of the project, the different types of processses, the algorithm implementation, the priority system, how to run the system, and more.