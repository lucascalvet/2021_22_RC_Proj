## Introdução

Este trabalho laboratorial foi desenvolvido no âmbito da Unidade Curricular de Redes de Computadores, tendo como objetivo implementar uma aplicação e protocolo que permitisse a transmissão de ficheiros através de uma porta série assíncrona, protegida de possíveis erros de transmissão.

Este relatório serve como complemento ao projeto, incluindo uma análise estatística da sua execução. O mesmo será divido nas seguintes secções:

- [Arquitetura](#arquitetura)
- [Estrutura do Código](#estrutura-do-código)
- [Casos de Uso Principais](#casos-de-uso-principais)
- [Protocolo de Ligação Lógica](#protocolo-de-ligação-lógica)
- [Protocolo de Aplicação](#protocolo-de-aplicação)
- [Validação](#validação)
- [Eficiência do protocolo de ligação de dados](#eficiência-do-protocolo-de-ligação-de-dados)
- [Conclusões](#conclusões)

## Arquitetura

### Camadas (Layers)

## Estrutura do Código

## Casos de Uso Principais

Para fazer a transferência de um ficheiro com writenoncanonical.c e readnoncanonical.c é necessário compilar e executar os referidos programas, em cada computador, respetivamente, e com a porta de série como argumento. No caso do write, será necessário também o argumento com o path do ficheiro a enviar e no caso do read, o utilizador poderá, facultativamente, definir o path do ficheiro de output como argumento. Ambos aceitam a flag `–v` que ativará os prints *verbose*, para debug.

Compilar **writenoncanonical.c** : `make writer`

Compilar **readnoncanonical.c** : `make reader`

Considerando, a título de exemplo, `wnc` como o executável resultante da compilação da writenoncanonical.c e `rnc` o executável resultante da compilação da readnoncanonical.c

Executar o binário referente ao **writenoncanonical.c** : 

wnc SerialPort InputFile [Verbose Mode]   
wnc /dev/ttyS<X> <file_path> [-v]

- X is the serial port number
- file_path is the path of the file to read and then send
- v (optional) enables verbose mode  

Exemplos: ```./wnc /dev/ttyS1 pinguim.gif\n``` ou ```./wnc /dev/ttyS1 pinguim.gif\n -v```

Executar o binário referente ao **readnoncanonical.c** : 

rnc SerialPort OutputFile [Verbose Mode]   
rnc /dev/ttyS<X> <file_path> [-v]

- X is the serial port number
- file_path is the path of the file to create
- v (optional) enables verbose mode  

Exemplos: ```./rnc /dev/ttyS1 pinguim.gif\n``` ou ```./rnc /dev/ttyS1 pinguim.gif\n -v```

---

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





## Protocolo de Ligação Lógica

O Protocolo de Ligação Lógica foi implementado nos ficheiros `link_layer.h` e `link_layer.c`. O objetivo deste protocolo é fornecer um serviço de comunicação de dados fiável entre dois sistemas ligados por um meio de transmissão, neste caso, um cabo de série. A transmissão é organizada em tramas, delimitadas por flags, que podem ser de Informação ou de Supervisão. As primeiras possuem um campo de dados para além dos headers comuns aos dois tipos. O protocolo controla os erros através de várias medidas como o campo BCC, isto é Block Check Character, um octeto tal que exista um número par de 1s em cada posição/bit. Outras medidas incluem os pedidos de retransmissão e a identificação de tramas de Informação repetidas. A transparência da transmissão é assegurada pela técnica de *byte stuffing*, que consiste em substituir os códigos, como por exemplo a flag, por dois bytes: o de escape e um `xor` entre o código e o octeto `0x20`. Vale notar que o byte de escape também deve ser codificado caso ocorra algures numa trama. Esta estratégia permite que os diversos códigos possam ocorrer nas tramas sem prejuízo na interpretação das mesmas, já que podemos inverter o processo e fazer *byte destuffing* para reaver o estado original de uma determinada trama. 

O protocolo assenta em 4 funções:

- `llopen` - Abrir portas
- `llwrite` - Escrever para portas
- `llread` - Ler informação recebida em portas
- `llclose` - Fechar portas

A acrescentar a estas funções, existem outras auxiliares que são necessárias para a implementação deste protocolo, tais como:

- `timeout_write` - Escreve uma determinada trama para a porta que lhe é fornecida e retorna a resposta que recebe
- `nc_read` - Lê uma trama na porta fornecida e manda a resposta adequada ao conteúdo que recebeu.
- `make_bcc` - Calcula o BCC para uma dada sequência de bytes
- `make_info` - Cria uma trama de informação, a partir dos dados que lhe são fornecidos
- `byte_stuffing_count` - Retorna o número de instâncias em que o *byte stuffing* é necessário na trama que lhe é fornecida (conta o número de códigos)
- `byte_destuffing_count` - Retorna o número de instâncias em que o *byte destuffing* é necessário na trama que lhe é fornecida (conta o número de bytes de escape)
- `byte_stuffing` - Aplica o processo de *byte stuffing* na trama que lhe é fornecida
- `byte_destuffing` - Aplica o processo de *byte destuffing* na trama que lhe é fornecida


## Protocolo de Aplicação

O Protocolo de Aplicação foi definido nos ficheiros `app.h` e `app.c`. A aplicação suporta dois tipos de pacotes: 

- **Pacotes de controlo** para sinalizar o início e o fim da transferência do ficheiro   
- **Pacotes de dados** contendo fragmentos do ficheiro a transmitir  

Assim, o Protocolo de Aplicação tem apenas a responsabilidade de criar e interpretar estes dois tipos de pacotes, "ignorando" a estrutura das tramas referentes ao Protocolo de Ligação Lógica. Para tal contém apenas quatro funções:

- `make_control_package`
- `read_control_package`
- `make_data_package`
- `read_data_package`

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
A combinação destas 4 funções permite a criação e o processamento dos pacotes do nível da aplicação. A utilização deste protocolo em conjunto com o de ligação lógica (que contém as funções relacionadas com o envio de tramas) permitem a transmissão e a receção dos dados do ficheiro.

## Validação

## Eficiência do protocolo de ligação de dados

## Conclusões


