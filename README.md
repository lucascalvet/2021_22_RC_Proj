## Sumário

Este trabalho laboratorial foi desenvolvido no âmbito da Unidade Curricular de Redes de Computadores, tendo como objetivo implementar uma aplicação e protocolo que permitissem a transmissão de ficheiros através de uma porta série assíncrona, protegida de possíveis erros de transmissão.

Os objetivos do tabalho, estabelecidos no guião, foram cumpridos com sucesso, tendo sido desenvolvida uma aplicação funcional, capaz de enviar qualquer tipo de ficheiro entre dois computadores, sem perda de dados.

## Introdução

O trabalho teve como objetivo desenvolver uma aplicação, suportada por um protocolo de dados, que, com recurso à comunicação por tramas de informação e através de uma porta série, tem a capacidade de transferir ficheiros entre computadores.

Este relatório serve como complemento ao projeto, incluindo uma análise estatística da sua execução. O mesmo está divido nas seguintes secções:

- [Arquitetura](#arquitetura): Identificação dos blocos funcionais e interfaces.
- [Estrutura do Código](#estrutura-do-código): Descrição das APIs, principais estruturas de dados, principais funções e a sua relação com a arquitetura.
- [Casos de Uso Principais](#casos-de-uso-principais): Identificação dos casos de uso e descrição da corrente de chamadas de funções.
- [Protocolo de Ligação Lógica](#protocolo-de-ligação-lógica): Identificação dos principais aspetos funcionais da ligação lógica e descrição da estratégia de implementação destes aspetos.
- [Protocolo de Aplicação](#protocolo-de-aplicação): Identificação dos principais aspetos funcionais da camada de aplicação e descrição da estratégia de implementação destes aspetos.
- [Validação](#validação): Descrição dos testes efetuados com apresentação quantificada dos resultados.
- [Eficiência do protocolo de ligação de dados](#eficiência-do-protocolo-de-ligação-de-dados): Caraterização estatística da eficiência do protocolo, efetuada recorrendo a medidas sobre o código desenvolvido.
- [Conclusões](#conclusões): Síntese da informação apresentada nas secções anteriores e reflexão sobre os objetivos de aprendizagem alcançados.

## Arquitetura

### Camadas de comunicação

### Compilação e execução



### Camadas (Layers)

## Estrutura do Código

## Casos de Uso Principais

Para utilizar os programas desenvolvidos, é necessário compilar os mesmos (através do comando `make`) e executá-los com os respetivos argumentos. De seguida encontram-se as instruções de execução dos mesmos.

Considera-se `wnc` como o executável resultante da compilação de `writenoncanonical.c` e `rnc` o executável resultante da compilação de `readnoncanonical.c`.

---

Instruções de execução do `wnc`: 

```  
./wnc serial_port file_path [-v]
```

- **serial_port**: obrigatório, indica o nº da porta de série
- **file_path**: obrigatório, indica o caminho do ficheiro a enviar
- **-v**: opcional, ativa o modo verboso  

Exemplos:

- `./wnc /dev/ttyS1 pinguim.gif`
- `./wnc /dev/ttyS0 images/picture.gif -v`

---

Instruções de execução do `rnc`:  

``` 
./rnc serial_port [file_path] [-v]
```

- **port**: obrigatório, indica o nº da porta de série
- **file_path**: opcional, indica o nome do ficheiro de destino (não estando definido, o ficheiro é guardado com o seu nome original)
- **-v**: opcional, ativa o modo verboso 

Exemplos:

- `./rnc /dev/ttyS1`
- `./rnc /dev/ttyS1 -v`
- `./rnc /dev/ttyS0 picture.gif -v`


<!---
O writenoncanonical.c irá primeiro ler e tratar dos argumentos. Depois de abrir o ficheiro dado para envio, irá abrir a porta lógica.

```c
  int port_fd = llopen(argv[1], TRANSMITTER);
  if (port_fd == -1)
    error(1, errno, "cannot open the serial port");
  if (port_fd == -2)
    error(1, 0, "no response after %d tries, cannot establish a connection", TRIES);
  if (port_fd == -3)
    error(1, 0, "got unexpected response, cannot establish a connection");
  if (port_fd < 0)
    error(1, 0, "error opening the serial port");
  if (verbose)
    printf("Connection established as the transmitter.\n");
```

De seguida, procedirar-se ao envio em si, preparando o pacote de controlo e fazendo write.

```c
  unsigned char *app_packet;
  int size, res;
  size = make_control_package(TRUE, file_info.st_size, basename(filepath), &app_packet);
  res = llwrite(port_fd, app_packet, size);
  free(app_packet);
```

 Se este não for recebido, irá tentar repetidas vezes, saindo sem sucesso passado número de TRIES, definido em macros. No outro caso, poderá proceder-se à escrita do ficheiro em si: num while loop, writenoncanonical lê MAX_DATA_SIZE – 4 bytes do ficheiro para um buffer, preparando então o pacote de dados e enviando. Além disso, manterá conta do tamanho dos dados já escritos.

 ```c
      size = make_data_package(seq_n, read_buffer, read_size, &app_packet);
      seq_n++;
      int llw_res = llwrite(port_fd, app_packet, size);

      if (llw_res < 0)
      {
        free(app_packet);
        error(1, 0, "error in llwrite");
      }

      write_file_size += size - 4;
```

Quando não ler mais dados do ficheiro, o loop de write irá terminar. Aí, escreverá um pacote de controlo final e chamará llclose, para fechar a porta de série e close, para fechar o ficheiro agora enviado.

Por sua vez, readnoncanonical.c passará um processo semelhante de interpretação de argumentos e abertura da porta série, esperando pelo pacote inicial de controlo de writenoncanonical.

```c
  while (!received_start_cp)
  {
    package_len = llread(port_fd, &package);
    if (package[0] == CP_START)
    {
      if(verbose) printf("Start Control Package Received\n");
      read_control_package(package, package_len, &file_size, &file_name);
      received_start_cp = TRUE;
    }
    free(package);
  }
```

Depois de abrir o ficheiro para onde fará output, readnoncanonical.c entrará em um loop para proceder à leitura dos pacotes de dados enviados.

```c
while (!end_package_stream)
  {
    if ((package_len = llread(port_fd, &package)) < 0)
    {
      free(package);
      error(1, errno, "llread failed");
    }

    switch (package[0])
    {
    case DP:
      package_len = read_data_package(package, &seq_n, &data);
      int written_size = write(file_fd, data, package_len);
      free(data);
      if (package_len <= 0 || written_size != package_len)
      {
        free(package);
        error(1, errno, "write to file failed");
      }
      read_file_size += written_size;
      break;
```

Quando receber o pacote de controlo final, verifica a integridade da escrita, verificando que o nome e tamanho do ficheiro escrito são congruentes com o recebido. Tal como write, fará llclose e close.
-->

## Protocolo de Ligação Lógica

O Protocolo de Ligação Lógica foi implementado nos ficheiros `link_layer.h` e `link_layer.c`. O objetivo deste protocolo é fornecer um serviço de comunicação de dados fiável entre dois sistemas ligados por um meio de transmissão, neste caso, um cabo de série. A transmissão é organizada em tramas que podem ser de dois tipos: 

- **Tramas de Supervisão** para comandos e respostas, com a estrutura: |F|A|C|BCC1|F| 

- **Tramas de Informação** que servem de *wrap* para os dados a serem transportados, com a estrutura: |F|A|C|BCC1|Campo de Dados (vários octetos)|BCC2|F|

  - F = Flag (octeto `01111110`)

  - A = Campo de Endereço que pode assumir os valores:
    - `0x03` para comandos enviados pelo Emissor e respostas
enviadas pelo Recetor
    - `0x01` para comandos enviados pelo Recetor e respostas
enviadas pelo Emissor

  - C = Campo de Controlo um octeto que identifica o tipo de comando ou resposta

  - BCC = Block Check Character

Por sua vez as tramas de Supervisão podem conter diferentes comandos e respostas:

- **SET** - Comando enviado pelo Emissor para estabelecer conexão com o Recetor. Se o Recetor comunicar de volta, dá se início à transferência de dados. Tem no Campo de Controlo o octeto `0x03`

- **UA** - Resposta não numerada que pode ser enviada por qualquer interveniente. Tem no Campo de Controlo o octeto `0x07`

- **RR** - Resposta positiva enviada pelo Recetor caso tenha recebido uma trama de Informação válida. Tem no Campo de Controlo o octeto `R0000101`, em que R pode ser 1 ou 0 de acordo com o número de sequência da trama de Informação recebida.

- **REJ** - Resposta negativa enviada pelo Recetor caso tenha recebido uma trama de Informação inválida. Tem no Campo de Controlo o octeto `R0000001`, em que R pode ser 1 ou 0 de acordo com o número de sequência da trama de Informação recebida.

- **DISC** - Comando de desconexão que deve ser enviado inicialmente pelo Emissor e posteriormente pelo Recetor de modo a sinalizar o fim da comunicação e a ser iniciado o processo de fecho da porta de série. Tem no Campo de Controlo o octeto `0x0B`


O protocolo controla os erros através de várias medidas como o campo BCC, isto é Block Check Character, um octeto tal que exista um número par de 1s em cada posição/bit. É resultado de `xor`'s sucessivos aplicados a cada byte protegido pelo BCC em questão. Outras medidas incluem os pedidos de retransmissão e a identificação de tramas de Informação repetidas. 

A transparência da transmissão é assegurada pela técnica de *byte stuffing*, que consiste em substituir os códigos, como por exemplo a flag, por dois bytes: o de escape e um `xor` entre o código e o octeto `0x20`. Vale notar que o byte de escape também deve ser codificado caso ocorra algures numa trama. Esta estratégia permite que os diversos códigos possam ocorrer nas tramas sem prejuízo na interpretação das mesmas, já que podemos inverter o processo e fazer *byte destuffing* para reaver o estado original de uma determinada trama. 

O protocolo assenta em 4 funções:

- `llopen` - Abrir a porta de série
- `llwrite` - Escrever para a porta
- `llread` - Ler informação recebida na porta
- `llclose` - Fechar a porta de série

A acrescentar a estas funções, existem outras auxiliares que são necessárias para a implementação deste protocolo, tais como:

- `timeout_write` - Escreve uma determinada trama para a porta que lhe é fornecida e retorna a resposta que recebe ou `NULL` se não recebeu uma resposta válida em nenhuma das tentativas que fez.
- `nc_read` - Lê uma trama na porta fornecida e manda a resposta adequada ao conteúdo que recebeu.
- `make_bcc` - Calcula o BCC para uma dada sequência de bytes, através da aplicação sucessiva de `xor`'s ao conjunto de bytes protegidos pelo referido BCC
- `make_info` - Cria uma trama de informação, a partir dos dados que lhe são fornecidos
- `byte_stuffing_count` - Retorna o número de instâncias em que o *byte stuffing* é necessário na trama que lhe é fornecida (conta o número de códigos)
- `byte_destuffing_count` - Retorna o número de instâncias em que o *byte destuffing* é necessário na trama que lhe é fornecida (conta o número de bytes de escape)
- `byte_stuffing` - Aplica o processo de *byte stuffing* na trama que lhe é fornecida
- `byte_destuffing` - Aplica o processo de *byte destuffing* na trama que lhe é fornecida

A função `timeout_write` é usada sempre que o Emissor envia um comando ou trama de Informação e necessita de esperar por uma resposta para continuar. Prentendemos que, após realizar o `write`, a função aguarde pela resposta e caso esta não seja recebida num determinado intervalo de tempo, a trama seja reenviada. Deste modo impedimos que ocorra um ciclo infinito e o protocolo torna-se mais robusto e resistente a falhas pontuais de comunicação. A função deve ainda realizar um número predefinido de tentativas (`tries`) destes reenvios antes de retornar `NULL`, resultando provavelmente na interrupção do protocolo. Para fazer este *loop*, utilizamos um `alarm` :

```c
// Como função global
void set_alarm()
{
  alarm_set = TRUE;
}

// No início da função timeout_write
(void)signal(SIGALRM, set_alarm);
write(fd, to_write, write_size);
alarm(TIMEOUT);

// No  final do loop que realiza os reads sucessivos e constrói a trama de resposta
if (alarm_set)
{
  alarm_set = FALSE;
  tries--;
  if (tries > 0)
  {
    write(fd, to_write, write_size);
    printf("Alarm triggered, trying again. %d tries left\n", tries);
    alarm(TIMEOUT);
  }
}
```
Por outro lado, a função `nc_read` é usada por parte do Recetor para ler e processar as tramas recebidas e enviar uma resposta adequada ao conteúdo ou comando recebido. Esta função inclui um ciclo onde realiza os `read`s e do qual só sai qundo lê uma trama válida, muito semelhante ao encontrado na `timeout_write`, exceto na ausência de *timeout* com `alarm`. Neste ciclo está implementado uma simples máquina de estados: inicialmente é ignorada qualquer informação até ser recebida a primeira flag; a máquina permanece neste segundo estado até receber um byte que não seja uma flag; por último são guardados todos os bytes que não sejam flags até ser recebida novamente uma flag. Se a informação recebida de facto se tratar de uma trama válida, a trama é processada e o seu tipo é identificado através de um `switch` com base no seu Campo de Controlo. No caso de ser um comando como SET ou DISC, a função limita-se a enviar a resposta adequada. Se, por outro lado, se tratar de uma trama de Informação, o segundo BCC é verificado, tal como já tinha sido verificado o primeiro e é feito o processo de *byte destuffing*, antes de enviar uma resposta. Em qualquer um dos casos, no final a trama lida é passada por referência para o argumento `read_package`.

```c
// O ciclo que lê a informação recebida na porta de série, utilizando uma máquina de estados simples
STOP = FALSE;
count = 0;
while (STOP == FALSE && !received)
{
  ua = FALSE;
  res = read(fd, &buf, 1);
  if (res)
  {
    if (buf == FLAG)
    {
      switch (flag_state)
      {
      case 0:
        flag_state = 1;
        break;
      case 2:
        STOP = TRUE;
        flag_state = 1;
        break;
      default:
        break;
      }
    }
    else
    {
      if (flag_state == 1)
      {
        flag_state = 2;
      }
      if (flag_state == 2)
      {
        packet[count] = buf;
        count++;
      }
    }
  }
}

// Processamento de uma trama de Informção
case C_INFO:
case C_INFO_N:
  sender_inf_count++;
  if ((n == 0 && packet[1] == C_INFO_N) || (n == 1 && packet[1] == C_INFO))
  {
    if (verbose)
      printf("Received unexpected sequence number data packet, possible duplicate. Sending RR. packet[1]:%x n:%x\n", packet[1], n);

    if (n)
    {
      response[2] = C_RR_N;
    }
    else
    {
      response[2] = C_RR;
    }

    received = FALSE;
    response[3] = response[1] ^ response[2];
    write(fd, response, write_size);
    receiver_rr_count++;
    break;
  }
  unsigned char *destuffed_info;
  if (verbose)
    printf("Count before BD: %d\n", count);
  count = byte_destuffing(packet, count, &destuffed_info);
  if (verbose)
    printf("Count after BD: %d\n", count);
  free(packet);
  packet = destuffed_info;
  if (make_bcc(&packet[3], count - 4) == packet[count - 1])
  {
    if (n)
    {
      n = 0;
      response[2] = C_RR;
    }
    else
    {
      n = 1;
      response[2] = C_RR_N;
    }
    if (verbose)
      printf("Info Body Checks Out. Sending RR and changing expected sequence number to %d.\n", n);
    receiver_rr_count++;
  }
  else
  {
    if (verbose)
      printf("Info Body Wrong. Sending REJ\n");
    if (n)
    {
      response[2] = C_REJ_N;
    }
    else
    {
      response[2] = C_REJ;
    }
    received = FALSE;
    response[3] = response[1] ^ response[2];
    write(fd, response, write_size);
    receiver_rej_count++;
  }
  break;

// Final da função nc_read
*read_package = packet;
return count;
}
```

A função `llopen` trata de configurar e abrir a porta de série. Recebe o identificador da porta e uma flag a indicar se se trata do Emissor (`TRANSMITTER`) ou do Recetor (`RECEIVER`). Faz uma configuração semelhante para os dois intervenientes, incluindo o `open` à porta. De seguida o Emissor envia o comando SET, ao qual o Recetor deverá responder com uma UA. Em caso de sucesso a função retorna o *file descriptor* da porta, caso contrário um número negativo que identifique o que correu mal.

```c
int llopen(char *port, enum Role flag)
{
  struct termios newtio;
  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  int fd = open(port, O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    return -1;
  }

  /* save current port settings */
  if (tcgetattr(fd, &oldtio) == -1)
  {
    return -1;
  }

  /* generate new port settings */
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* non-blocking */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    return -1;
  }

  if (flag == TRANSMITTER)
  {
    unsigned char set[5];
    set[0] = FLAG;
    set[1] = A_SENDER;
    set[2] = C_SET;
    set[3] = set[1] ^ set[2];
    set[4] = FLAG;

    unsigned char *response;
    response = timeout_write(fd, set, 5);
    sender_set_count++;
    if (response == NULL) // There was no response after a set number of tries
    {
      free(response);
      return -2;
    }
    if (response[1] != C_UA) // Got unexpected response
    {
      free(response);
      return -3;
    }
    receiver_ua_count++;
    free(response);
  }
  else if (flag == RECEIVER)
  {
    unsigned char *request;
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_SET)
    {
      free(request);
      printf("Got wrong instruction, expected SET.\n");
      return -1;
    }
    free(request);
    printf("Receiver - Connection established.\n");
  }

  return fd;
}
```

A função `llwrite` escreve uma dada informação (passada no argumento `buffer` com um tamanho `length`) para a porta de série, colocando-a numa trama de Informação. A função auxiliar `make_info` coloca a informação a transmitir numa trama de Informação, acrescentando o *header* característico deste tipo de tramas e numerando-as. A `llwrite` recorre à `timeout_write` e tenta reenviar a informação caso receba uma resposta REJ ou uma RR não condizente com o número de sequência esperado.

```c
int llwrite(int fd, unsigned char *buffer, int length)
{
  unsigned char *info_frame;
  unsigned char *response;

  int size = make_info(buffer, length, n, &info_frame);
  int try_again;
  do
  {
    sender_inf_count++;

    try_again = FALSE;
    response = timeout_write(fd, info_frame, size);
    if (response == NULL) // There was no response after a set number of tries
    {
      free(response);
      return -2;
    }

    if (response[1] == C_REJ_N || response[1] == C_REJ || (response[1] == C_RR_N && n) || (response[1] == C_RR && !n))
    {
      try_again = TRUE;
      if (response[1] == C_REJ_N || response[1] == C_REJ)
      {
        receiver_rej_count++;
        if (verbose)
          printf("[llwrite] Data frame rejected by receiver, trying again...\n");
      }
      else
      {
        receiver_rr_count++;
        if (verbose)
          printf("[llwrite] Data frame requested again, trying again... response[1]:%x n:%x\n", response[1], n);
      }

      free(response);
    }

  } while (try_again);

  if (response[1] != C_RR_N && response[1] != C_RR) // Got unexpected response
  {
    free(response);
    error(1, 0, "Wrong response.\n");
  }
  else
  {
    if (n)
    {
      n = 0;
    }
    else
    {
      n = 1;
    }
    receiver_rr_count++;
    free(response);
  }

  return size;
}
```

A função `llread` lê uma trama de Informação e retira por referência o Campo de Dados da mesma para o argumento `buffer`. É resumidamente uma *wrapper* da função `nc_read` que já lê e processa a trama recebida pela porta de série e responde apropriadamente. A `llread` retorna um número negativo que identifica o erro, se o resultado da `nc_read` não for o esperado. Em caso de sucesso, retorna o tamanho do `buffer` após ser preenchido com o Campo de Dados da trama de Informação recebida.

```c
int llread(int fd, unsigned char **buffer)
{
  unsigned char *request;
  int size = nc_read(fd, &request);
  if (request == NULL || size == 0)
  {
    error(1, 0, "nc_read() returned NULL, this should not happen\n");
  }
  if (request[1] != C_INFO && request[1] != C_INFO_N)
  {
    free(request);
    printf("Got wrong instruction, expected INFO.");
    return -1;
  }

  *buffer = (unsigned char *)malloc((size - 4) * sizeof(unsigned char));
  memcpy(*buffer, &request[3], size - 4);
  free(request);
  return size - 4;
}
```

A função `llclose` trata de fechar a porta de série. Recebe o *file descriptor* da porta e uma flag a indicar se se trata do Emissor (`TRANSMITTER`) ou do Recetor (`RECEIVER`). Para começar o Emissor deve mandar o comando DISC, ao qual o Recetor deve responder com um comando DISC também. O Emissor deve, depois, mandar um UA. Finalmente é chamada a função `close` para fechar a porta de série, após *resetar* as definições da porta para as anteriores à execução da `llopen`. Se alguma parte deste processo falhar, a função `llclose` retorna um número negativo identificativo do passo que falhou. Se tudo correr conforme o esperado 0 é o valor retornado.

```c
int llclose(int fd, enum Role flag)
{
  printf("Closing connection...\n");

  if (flag == TRANSMITTER)
  {
    unsigned char disc[5];
    disc[0] = FLAG;
    disc[1] = A_SENDER;
    disc[2] = C_DISC;
    disc[3] = disc[1] ^ disc[2];
    disc[4] = FLAG;

    unsigned char *response;
    response = timeout_write(fd, disc, 5);
    sender_disc_count++;
    if (response == NULL)
    {
      free(response);
      printf("No response to DISC after %d tries.\n", TRIES);
      return -1;
    }
    if (response[1] != C_DISC)
    {
      free(response);
      printf("Wrong response, expected DISC.\n");
      return -1;
    }
    receiver_disc_count++;
    free(response);

    unsigned char ua[5];
    ua[0] = FLAG;
    ua[1] = A_RECEIVER;
    ua[2] = C_UA;
    ua[3] = ua[1] ^ ua[2];
    ua[4] = FLAG;
    if (write(fd, ua, 5) != 5)
    {
      error(0, errno, "error writing UA");
      return -1;
    }
    sender_ua_count++;

    printf("Transmitter - Connection closed.\n");
  }
  else if (flag == RECEIVER)
  {
    unsigned char *request;
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_DISC)
    {
      free(request);
      printf("Got wrong instruction, expected DISC.");
      return -1;
    }
    free(request);
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_UA)
    {
      free(request);
      printf("Got wrong instruction, expected UA.");
      return -1;
    }
    free(request);
    printf("Receiver - Connection closed.\n");
  }

  sleep(1);

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  if (close(fd) == -1)
  {
    error(0, errno, "error closing the serial port");
    return -1;
  }

  return 0;
}
```

Resumidamente, o Protocolo de Ligação Lógica permite estabelecer uma conexão entre duas máquinas, através de uma porta de série e transferir informação entre elas, de uma forma eficiente, transparente e segura. Tudo isto com mecanismos de mitigação de erros e falhas de conexão. Em conjunto com uma camada de Aplicação, poderemos usar este protocolo para enviar um ficheiro entre duas máquinas.


## Protocolo de Aplicação

O Protocolo de Aplicação foi definido nos ficheiros `app.h` e `app.c`. A aplicação suporta dois tipos de pacotes: 

- **Pacotes de controlo** para sinalizar o início e o fim da transferência do ficheiro   
- **Pacotes de dados** contendo fragmentos do ficheiro a transmitir  

O ficheiro a ser enviado será dividido em pacotes de dados e estes pacotes serão divididos em tramas de Informação para serem enviados de acordo com o Protocolo de Ligação Lógica. Assim, o Protocolo de Aplicação tem apenas a responsabilidade de criar e interpretar estes dois tipos de pacotes, "ignorando" a estrutura das tramas referentes ao Protocolo de Ligação Lógica. Para tal contém quatro funções:

- `make_control_package` - Cria pacotes de controlo
- `read_control_package` - Processa pacotes de controlo
- `make_data_package` - Cria pacotes de dados
- `read_data_package` - Processa pacotes de dados

A função `make_control_package` cria os pacotes de controlo. Recebe um booleano, `start`, que indica se se trata de um pacote de sinalização de início (com '2' no campo de controlo) ou de final (com '3' no campo de controlo). Além disso, recebe o tamanho do ficheiro a ser transmitido e o nome do mesmo (nos argumentos `file_size` e `file_name` respetivamente), parâmetros que vão ser codificados com o formato TLV no pacote de controlo. O campo Type será 0 para o tamanho do ficheiro e 1 para o nome, por convenção. O pacote de controlo resultante será passado por referência para o argumento `control_package`. Esta função retorna o tamanho do pacote de controlo criado.

```c
int make_control_package(int start, int file_size, char *file_name, unsigned char **control_package)
{
    unsigned char *result_package = (unsigned char *)malloc(MAX_PACKET_SIZE * sizeof(unsigned char));
    if (start)
    {
        result_package[0] = CP_START;
    }
    else
    {
        result_package[0] = CP_END;
    }

    result_package[1] = CP_T_FSIZE;
    result_package[2] = sizeof(int);
    
    memcpy(&result_package[3], &file_size, sizeof(int));

    int index = 3 + sizeof(int);

    result_package[index++] = CP_T_FNAME;
    result_package[index++] = strlen(file_name) + 1;

    for (int i = 0; i < strlen(file_name) + 1; i++)
    {
        result_package[index + i] = file_name[i];
    }
    
    *control_package = result_package;
    return index + strlen(file_name) + 1;
}
```

Por sua vez, a função `read_control_package` lê pacotes de controlo e interpreta as informações que estes fornecem. O argumento `control_package` representa o pacote de controlo a ser lido e `package_size` o seu tamanho em bytes. É verificado se o campo de controlo do pacote é válido, tal como se os campos Type são os esperados. Caso o pacote a ser lido seja de facto validado, os seus campos TLV são lidos, através de `memcpy`'s  com os tamanhos indicados em cada campo Length. O tamanho do ficheiro indicado pelo pacote é guardado por referência no inteiro `file_size` e o nome no argumento `file_name`. É retornado o tamanho do pacote de controlo lido.

```c
int read_control_package(unsigned char *control_package, int package_size, int *file_size, char **file_name)
{
    if (control_package[0] != CP_START && control_package[0] != CP_END)
    {
        printf("Not a control package inside read_control_package!\n");
        return -1;
    }

    int fname_len = 0;
    int fsize_len = 0;
    for (int i = 0; i < package_size; i++)
    {
        if (control_package[i] == CP_T_FSIZE)
        {
            fsize_len = control_package[++i];

            memcpy(file_size, &control_package[++i], fsize_len);
            i += fsize_len;
        }

        if (control_package[i] == CP_T_FNAME)
        {
            fname_len = control_package[++i];
            *file_name = (char *)malloc(fname_len * sizeof(char));

            memcpy(*file_name, &control_package[++i], fname_len);
            i += fname_len;
        }
    }

    return 5 + fsize_len + fname_len;
}
```

A função responsável por criar os pacotes de dados é a `make_data_package`. Numerar os pacotes de dados é outra responsabilidade desta função, como tal, após colocar o campo de controlo do novo pacote a 1, define o número de sequência como o módulo de 256 do contador que lhe é passado na variável `seq_n`. O tamanho do pacote é escrito em dois bytes, seguido da informação propriamente dita, ou seja, do fragmento do ficheiro a ser transmitido naquele pacote (passado no argumento `data`). O pacote de dados que resulta deste processo é passado por referência para o argumento `data_package`. O valor retornado corresponde ao tamanho o pacote de dados criado.

```c
int make_data_package(int seq_n, unsigned char *data, int size, unsigned char **data_package)
{
    unsigned char *result_package = (unsigned char *)malloc((size + 4) * sizeof(unsigned char));
    result_package[0] = DP;
    result_package[1] = seq_n % 256;
    result_package[2] = size / 256;
    result_package[3] = size % 256;

    for (int i = 0; i < size; i++)
    {
        result_package[i + 4] = data[i];
    }

    *data_package = result_package;
    return size + 4;
}
```

Por último, a função `read_data_package` lê e interpreta um pacote de dados. Inicialmente é verificado o campo de controlo do pacote fornecido como argumento, o `data_package`. Se o pacote passar nesta verificação, o número de sequência do pacote é guardado no argumento `seq_n`. Finalmente o campo de dados, cujo tamanho é calculado na expressão `data_package[2] * 256 + data_package[3]`, é lido e passado por referência para o argumento `data`. A função retorna o tamanho do campo de dados do pacote que leu.

```c
int read_data_package(unsigned char *data_package, int *seq_n, unsigned char **data)
{
    if (data_package[0] != DP)
    {
        printf("Not a data package inside read_data_package!\n");
        return -1;
    }
    *seq_n = data_package[1];
    int data_size = data_package[2] * 256 + data_package[3];

    unsigned char *result_data = (unsigned char *)malloc(data_size * sizeof(unsigned char));

    for (int i = 0; i < data_size; i++)
    {
        result_data[i] = data_package[i + 4];
    }

    *data = result_data;
    return data_size;
}
```
A combinação destas 4 funções permite a criação e o processamento dos pacotes do nível da aplicação. A utilização deste protocolo em conjunto com o de ligação lógica (que contém as funções relacionadas com o envio de tramas) permite a transmissão e a receção dos dados do ficheiro.

## Validação

## Eficiência do protocolo de ligação de dados

## Conclusões


