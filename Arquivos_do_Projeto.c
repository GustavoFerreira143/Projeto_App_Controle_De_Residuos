#include <gtk/gtk.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <curl/curl.h>
#include <glib.h>
#include ".\cJSON/cJSON.h"
#include <math.h>

#define MAX_CARACTERES 50
#define URL_UPLOAD_DATA "http://127.0.0.1/jwt_server/upload.php"
#define TOKEN_URL "http://127.0.0.1/jwt_server/token.php"
#define DOWNLOAD_URL "http://localhost/jwt_server/Dowload_arquivos.php"

//------------------------------------------------------------------------variaveis_globais--------------------------------------------------------------------------------------------------

GtkBuilder *builder;
typedef struct
{
	char email[100];
	char senha[100];
} Usuario;

Usuario usuario_atual;
char valor[100];
char Nome[100];
char Tipo[100];
char Arquivo[100];
char id[20];
char Bairro[70];
char Cidade[70];
char Estado[4];
int geral = 0 ;
guint timer_id = 0;
guint carregar_chat = 0;
char hora_mensagem[256];
char Datatabela[250];
char id_chamado[250];
char id_mensagem[250];
char id_relatorioEmpresa[250];
char id_relatorioADM[250];


//---------------------------------------------------------------------Fim das Variaveis Globais------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------caixa de Avisos---------------------------------------------------------------------------------------------------------------------------------

void mensagem(char text[100], char secondary_text[100], int valor, char complemento[500])
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Erro_Logout"));
	gtk_message_dialog_format_secondary_text(mensagem_dialogo, secondary_text);
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);

	if(valor == 1)
	{
		char texto_final[200];
		snprintf(texto_final, sizeof(texto_final), "%s %s",text,complemento);
		gtk_message_dialog_set_markup(mensagem_dialogo,texto_final);
		gtk_widget_set_name(dialogo_widget, "Caixa_de_Logado");
	}
	else
	{
		gtk_message_dialog_set_markup(mensagem_dialogo,text);
		gtk_widget_set_name(dialogo_widget, "Caixa_de_Erro");
	}

	gtk_widget_queue_draw(dialogo_widget);


	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
}

//-------------------------------------------------------------------Funcao para formatar Strings e Dowload e Envio de Arquivos------------------------------------------------------------------------------------------

struct MemoryStruct
{
	char *memory;
	size_t size;
};
// Callback para gravar a resposta do servidor
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL)
	{
		printf("Não foi possível alocar memória.\n");
		free(mem->memory); // Liberando memória alocada previamente
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0; // Null-terminando o buffer

	return realsize;
}

void formatar_mensagem(const gchar *mensagem_original, gchar *mensagem_formatada, size_t tamanho_maximo)
{
	if (mensagem_original == NULL || mensagem_formatada == NULL || tamanho_maximo == 0)
	{
		return; // Verifica se os ponteiros são válidos
	}

	// Remove o trecho entre "-----" no começo e no final, se presente
	const gchar *inicio = strstr(mensagem_original, "--");
	if (inicio != NULL)
	{
		inicio += 5; // Avança para depois do "-----"
		const gchar *fim = strstr(inicio, "---");
		if (fim != NULL)
		{
			mensagem_original = fim + 5; // Avança para depois do segundo "-----"
		}
	}


	// Encontra o primeiro '\n' após o '\n\n' e ignora o "HH:MM" após ele
	gchar *pos_hora = strchr(mensagem_original, '\n');
	if (pos_hora != NULL)
	{
		*pos_hora = '\0'; // Termina a string antes do horário
	}

	// Cópia e formatação da mensagem
	size_t comprimento = strlen(mensagem_original);
	if (comprimento >= tamanho_maximo)
	{
		// Copia até o limite máximo e adiciona "..."
		strncpy(mensagem_formatada, mensagem_original, tamanho_maximo - 4); // -4 para "..."
		mensagem_formatada[tamanho_maximo - 4] = '\0'; // Termina a string
		strcat(mensagem_formatada, "..."); // Adiciona "..."
	}
	else
	{
		// Cópia normal da mensagem
		strncpy(mensagem_formatada, mensagem_original, tamanho_maximo);
		mensagem_formatada[tamanho_maximo - 1] = '\0'; // Garante a terminação da string
	}
}


void formatar_valor_em_reais(const char *valor, char *buffer, size_t tamanho)
{
	// Separação das partes inteira e decimal do valor recebido como string
	char inteiro[50] = {0};
	char decimal[3] = {0};  // Limita a duas casas para os centavos
	int i = 0, j = 0;

	// Extrai a parte inteira até o ponto ou vírgula
	while (valor[i] != '.' && valor[i] != ',' && valor[i] != '\0')
	{
		inteiro[i] = valor[i];
		i++;
	}

	// Se houver parte decimal, extrai e limita a duas casas decimais
	if (valor[i] == '.' || valor[i] == ',')
	{
		i++;  // Pula o ponto ou vírgula
		while (valor[i] != '\0' && j < 2)
		{
			decimal[j++] = valor[i++];
		}
	}

	// Ajuste para garantir que haja exatamente duas casas decimais
	if (j == 0)
	{
		strcpy(decimal, "00");  // Nenhum decimal presente, adiciona "00"
	}
	else if (j == 1)
	{
		decimal[1] = '0';  // Uma casa decimal presente, adiciona "0" à direita
	}

	// Converte a parte inteira para um inteiro longo para formatação
	long long int parte_inteira = strtoll(inteiro, NULL, 10);

	// Prepara para a formatação final no estilo brasileiro
	char temp[100];
	char *p = temp + sizeof(temp) - 1; // Ponteiro para o final do buffer
	*p = '\0'; // Terminador de string
	int count = 0;

	// Formata a parte inteira com separadores de milhar
	do
	{
		if (count > 0 && count % 3 == 0)
		{
			*(--p) = '.'; // Adiciona o separador de milhar
		}
		*(--p) = '0' + (parte_inteira % 10); // Adiciona o dígito
		parte_inteira /= 10;
		count++;
	}
	while (parte_inteira > 0);

	// Usa snprintf para criar a string final no buffer com a parte decimal formatada
	snprintf(buffer, tamanho, "R$ %s,%s", p, decimal);
}



char *str_replace(const char *orig, const char *rep, const char *with)
{
	char *result;    // String resultante
	char *ins;       // Posição atual na string original
	char *tmp;       // Para armazenar posições temporárias
	int len_rep;     // Comprimento da string a ser substituída
	int len_with;    // Comprimento da string substituta
	int len_front;   // Comprimento até a parte da string antes da ocorrência encontrada
	int count;       // Contador de quantas vezes a string aparece

	// Se rep ou orig for NULL, ou se rep for uma string vazia, retorne a original
	if (!orig || !rep)
		return NULL;
	len_rep = strlen(rep);
	if (len_rep == 0)
		return NULL;
	if (!with)
		with = ""; // Se with for NULL, trate como uma string vazia
	len_with = strlen(with);

	// Contar quantas vezes a string rep aparece em orig
	ins = (char *)orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count)
	{
		ins = tmp + len_rep;
	}

	// Alocar memória para a nova string resultante
	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
		return NULL;

	// Substituir cada ocorrência
	while (count--)
	{
		ins = strstr(orig, rep);    // Encontrar a próxima ocorrência de rep
		len_front = ins - orig;     // Distância até a ocorrência encontrada
		tmp = strncpy(tmp, orig, len_front) + len_front; // Copiar parte antes da ocorrência
		tmp = strcpy(tmp, with) + len_with;              // Copiar string substituta
		orig += len_front + len_rep;                     // Avançar para a próxima parte
	}
	strcpy(tmp, orig); // Copiar o restante da string original
	return result;
}



// Callback para mostrar o progresso do download
static int progress_callback(void *ptr, double TotalToDownload, double NowDownloaded,
                             double TotalToUpload, double NowUploaded)
{
	if (TotalToDownload > 0)
	{
		int percent = (int)((NowDownloaded / TotalToDownload) * 100);
		fflush(stdout);
	}
	return 0; // Retorna 0 para continuar o download
}

// Fun��o para obter o token
void obter_token(char *token, size_t token_size)
{
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size = 0;

	if (chunk.memory == NULL)
	{
		fprintf(stderr, "Falha ao alocar mem�ria.\n");
		return;
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, TOKEN_URL);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		const char *data = "{}";  // Corpo JSON vazio
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
		}
		else
		{
			char *token_start = strstr(chunk.memory, "\"token\":\"");
			if (token_start)
			{
				token_start += 9; // Pular a string `"token":"`
				char *token_end = strchr(token_start, '"');
				if (token_end)
				{
					*token_end = '\0'; // Termina a string
					strncpy(token, token_start, token_size - 1);
					token[token_size - 1] = '\0'; // Assegura que a string est� terminada
				}
			}
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	free(chunk.memory);
	curl_global_cleanup();
}

int enviar_dados_usuario(const char *token,char dados_usuario[256])
{
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk;

	// Inicializar a estrutura de memória para a resposta do servidor
	chunk.memory = malloc(1);
	chunk.size = 0;

	if (chunk.memory == NULL)
	{
		fprintf(stderr, "Falha ao alocar memória.\n");
		return -1;
	}

	// Buffer para armazenar o dado digitado pelo usuário



	curl = curl_easy_init();
	if (curl)
	{
		// Define a URL para upload dos dados
		curl_easy_setopt(curl, CURLOPT_URL, URL_UPLOAD_DATA);

		// Define cabeçalhos de autenticação
		struct curl_slist *headers = NULL;
		char auth_header[256];
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
		headers = curl_slist_append(headers,"Content-Type: application/json; charset=UTF-8");
		headers = curl_slist_append(headers, auth_header);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Envia os dados digitados pelo usuário usando POST
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dados_usuario); // Envia os dados
		printf("Dados enviados: %s\n", dados_usuario); // Imprime os dados que serão enviados

		// Configurar callback para capturar resposta do servidor
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

		// Executa a solicitação
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fprintf(stderr, "Falha na solicitação CURL: %s\n", curl_easy_strerror(res));
		}
		else
		{
			printf("Resposta do servidor: %s\n", chunk.memory);
		}

		// Libera recursos
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}

	// Libera a memória da resposta
	free(chunk.memory);
	return 0;
}

void baixar_arquivo(char *token, char *nome_arquivo, char *extensao, gchar *Caminho)
{
	char *nome_arquivo_formatado = str_replace(nome_arquivo, ":", "-");
	nome_arquivo_formatado = str_replace(nome_arquivo_formatado, " ", "_");

	CURL *curl;
	CURLcode res;
	FILE *arquivo;
	char url[512];
	char caminho_completo[1024];  // Caminho completo para o arquivo com diretório e extensão

	// Cria a URL de download usando o nome do arquivo fornecido
	snprintf(url, sizeof(url), "http://127.0.0.1/jwt_server/Dowload_arquivos.php?file=%s", nome_arquivo_formatado);

	// Cria o caminho completo do arquivo, incluindo diretório e extensão
	snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s.%s", Caminho, nome_arquivo_formatado, extensao);

	// Verifica se o arquivo já existe
	if (access(caminho_completo, F_OK) == -1)
	{
		// Se o arquivo não existir, cria um arquivo vazio
		arquivo = fopen(caminho_completo, "wb");
		if (!arquivo)
		{
			perror("Erro ao criar o arquivo para download");
			return;
		}
		fclose(arquivo);
	}

	// Abre o arquivo local para salvar o download
	arquivo = fopen(caminho_completo, "wb");
	if (!arquivo)
	{
		perror("Erro ao abrir o arquivo para download");
        char Caminhodoarquivo[400];
        snprintf(Caminhodoarquivo,sizeof(Caminhodoarquivo),"Caminho %s, não encontrado",caminho_completo);
		mensagem("Erro no Download",Caminhodoarquivo,0,"") ;
		return;
	}

	curl = curl_easy_init();
	if (curl)
	{
		struct curl_slist *headers = NULL;
		char auth_header[512];
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);

		headers = curl_slist_append(headers, auth_header);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, arquivo);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
		}
		else
		{
			char Caminhodoarquivo[400];
			snprintf(Caminhodoarquivo,sizeof(Caminhodoarquivo),"Baixado em %s",caminho_completo);
			mensagem("Dowload Comcluido com Sucesso",Caminhodoarquivo,1,"");
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		fclose(arquivo);
	}

	// Lê e formata o conteúdo com base na extensão
	FILE *arquivo_origem = fopen(caminho_completo, "rb");
	if (!arquivo_origem)
	{
		perror("Erro ao reabrir o arquivo para formatação");
		return;
	}

	fseek(arquivo_origem, 0, SEEK_END);
	long tamanho = ftell(arquivo_origem);
	rewind(arquivo_origem);

	char *conteudo = malloc(tamanho + 1);
	fread(conteudo, 1, tamanho, arquivo_origem);
	conteudo[tamanho] = '\0';
	fclose(arquivo_origem);

	// Formata o conteúdo com base na extensão
	char *conteudo_formatado = NULL;
	if (strcmp(extensao, "txt") == 0)
	{
		conteudo_formatado = str_replace(conteudo, ";", "");
		conteudo_formatado = str_replace(conteudo_formatado, ":", "\n");
	}
	else if (strcmp(extensao, "csv") == 0 || strcmp(extensao, "xls") == 0)
	{
		conteudo_formatado = str_replace(conteudo, ":", "\n");
	}
	else
	{
		conteudo_formatado = strdup(conteudo);
	}

	// Salva o conteúdo formatado
	FILE *arquivo_destino = fopen(caminho_completo, "wb");
	if (arquivo_destino)
	{
		fwrite(conteudo_formatado, 1, strlen(conteudo_formatado), arquivo_destino);
		fclose(arquivo_destino);
	}
	else
	{
		perror("Erro ao abrir o arquivo para salvar o conteúdo formatado");
	}

	free(conteudo);
	free(conteudo_formatado);
}



int hash_user_password(const char *password, char *hashed_password)
{
	if (sodium_init() < 0)
	{
		return -1;
	}

	if (crypto_pwhash_str(hashed_password, password, strlen(password),
	                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
	                      crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
	{
		return -1;
	}

	return 0;
}

// Função para verificar a senha
int verify_password(const char *password, const char *hashed_password)
{
	if (crypto_pwhash_str_verify(hashed_password, password, strlen(password)) != 0)
	{
		return 0; // Senha não coincide
	}
	return 1; // Senha coincide
}










int string_somente_numeros(const char *str)
{
	int i = 0;

	// Verifica se a string está vazia
	if (str[0] == '\0')
	{
		return 0; // Retorna falso se a string estiver vazia
	}

	// Itera por todos os caracteres da string
	while (str[i] != '\0')
	{
		if (!isdigit(str[i]))   // Verifica se o caractere atual não é um dígito
		{
			return 0; // Retorna falso se encontrar algo que não seja número
		}
		i++;
	}

	return 1; // Retorna verdadeiro se todos os caracteres forem números
}




//-------------------------------------------------------------------------------------//---------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------Conex�o com banco de dados--------------------------------------------------------------------------------------------------
MYSQL *_stdcall obterConexao()
{

	char *servidor = "127.0.0.1";
	char *usuario = "root";
	char *senha = "";
	char *nomeBanco = "projeto_ambiental";

	MYSQL *conexao = mysql_init(NULL);

	if(!mysql_real_connect(conexao, servidor, usuario,senha, nomeBanco, 3306,NULL, 0))
	{
		g_print("Não foi Possivel conectar ao Banco de Dados Verifique se o banco está devidamente ligado ip do banco cadastrado : 127.0.0.1, usuario root senha :'' , banco projeto_ambiental ") ;
		mysql_close(conexao);
	}
	else
	{
		if (mysql_set_character_set(conexao, "utf8"))
		{
			fprintf(stderr, "Erro ao configurar UTF-8: %s\n", mysql_error(conexao));

		}
		return conexao;
	}
}


//-------------------------------------------------------------------Fim da fun��o de conex�o------------------------------------------------------------------------------------------------



// --------------------------------------------------------------Inicio da Fun��o para carregar o CSS----------------------------------------------------------------------------------------
void carregar_css(void)
{
	GtkCssProvider *provider = gtk_css_provider_new();
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	// Carregar o arquivo CSS
	gtk_css_provider_load_from_path(provider, "./glade/css/Projeto.css", NULL);

	// Aplicar o estilo globalmente
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	g_object_unref(provider);
}
// ------------------------------------------------------------------Fim da Fun��o para carregar o CSS----------------------------------------------------------------------------------------


//---------------------------------------------------------------------Funcão de atualização---------------------------------------------------------------------------------------------------------------------------------

gboolean atualizar_tabelas(GtkLabel *label)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *lista1 = NULL, *lista2 = NULL, *lista3 = NULL, *lista4 = NULL, *mansagens_pendentes = NULL;
	MYSQL_ROW row;

	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));
	GtkListStore *store2 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore2"));
	GtkListStore *store3 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore3"));
	GtkListStore *store4 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore4"));


	GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
	GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
	GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);



	char list1[250];
	char list2[250];
	char list3[250];
	char list4[250];


	snprintf(list1, sizeof(list1), "SELECT nome_arquivo, data, id_relatorio FROM relatorio_valores WHERE empresa = '%s' AND id_relatorio > %s ",Nome,id_relatorioEmpresa);
	if (mysql_query(conexao, list1) == 0)
	{
		lista1 = mysql_store_result(conexao);
	}
	if (lista1 != NULL && mysql_num_rows(lista1) > 0)
	{
		while ((row = mysql_fetch_row(lista1)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(store1, &iter);
			gtk_list_store_set(store1, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, pixbuf_xls, 3,pixbuf_csv, 4, pixbuf_txt,-1);
			snprintf(id_relatorioEmpresa,sizeof(id_relatorioEmpresa),"%s",row[2]);
		}
		mysql_free_result(lista1);
	}


	snprintf(list2, sizeof(list2), "SELECT titulo_mensagem, corpo_mensagem, usuario_criador, data_de_criacao, id_mensagem FROM mensagens WHERE empresa_destino = '%s' AND id_mensagem > %s ORDER BY id_mensagem DESC",Nome,id_mensagem);


	if (mysql_query(conexao, list2) == 0)
	{
		lista2 = mysql_store_result(conexao);
	}

	if (lista2 != NULL && mysql_num_rows(lista2) > 0)
	{
		int x=0;
		while ((row = mysql_fetch_row(lista2)) != NULL)
		{
			if(x==0)
			{
				snprintf(id_mensagem,sizeof(id_mensagem),"%s",row[2]);
			}
			GtkTreeIter list2_iter;
			char titulo[700];
			snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
			gtk_list_store_append(store2, &list2_iter);
			gtk_list_store_set(store2, &list2_iter, 0, titulo, -1);


			gtk_list_store_append(store2, &list2_iter);
			gtk_list_store_set(store2, &list2_iter, 0, row[1],-1);

			char empresa[200];
			gtk_list_store_append(store2, &list2_iter);
			snprintf(empresa, sizeof(empresa), "Criado por: %s ",row[2]);
			gtk_list_store_set(store2, &list2_iter, 0, empresa,-1);

			char data[200];
			gtk_list_store_append(store2, &list2_iter);
			snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
			gtk_list_store_set(store2, &list2_iter, 0, data,-1);

			char divisoria[800];
			gtk_list_store_append(store2, &list2_iter);
			snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
			gtk_list_store_set(store2, &list2_iter, 0, divisoria,-1);
			x++;
		}
		mysql_free_result(lista2);
	}


	snprintf(list3, sizeof(list3), "SELECT titulo_chamado, corpo_chamado, nome_empresa, data_criacao, id_chamado FROM chamados WHERE id_chamado > %s ORDER BY id_chamado DESC",id_chamado);
	if (mysql_query(conexao, list3) == 0)
	{
		lista3 = mysql_store_result(conexao);
	}

	if (lista3 != NULL && mysql_num_rows(lista3) > 0)
	{
		int x=0;
		while ((row = mysql_fetch_row(lista3)) != NULL)
		{
			if(x == 0)
			{
				snprintf(id_chamado,sizeof(id_chamado),"%s",row[4]);
			}

			GtkTreeIter list3_iter;
			char titulo[700];
			snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
			gtk_list_store_append(store3, &list3_iter);
			gtk_list_store_set(store3, &list3_iter, 0, titulo, -1);


			gtk_list_store_append(store3, &list3_iter);
			gtk_list_store_set(store3, &list3_iter, 0, row[1],-1);

			char empresa[200];
			gtk_list_store_append(store3, &list3_iter);
			snprintf(empresa, sizeof(empresa), "Empresa Referente: %s ",row[2]);
			gtk_list_store_set(store3, &list3_iter, 0, empresa,-1);

			char data[200];
			gtk_list_store_append(store3, &list3_iter);
			snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
			gtk_list_store_set(store3, &list3_iter, 0, data,-1);

			char divisoria[800];
			gtk_list_store_append(store3, &list3_iter);
			snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
			gtk_list_store_set(store3, &list3_iter, 0, divisoria,-1);
			x++;

		}
		mysql_free_result(lista3);
	}


	snprintf(list4, sizeof(list4), "SELECT nome_arquivo, data, empresa, id_relatorio FROM relatorio_valores WHERE id_relatorio > %s ",id_relatorioADM);
	if (mysql_query(conexao, list4) == 0)
	{
		lista4 = mysql_store_result(conexao);
	}

	if (lista4 != NULL && mysql_num_rows(lista4) > 0)
	{
		while ((row = mysql_fetch_row(lista4)) != NULL)
		{
			GtkTreeIter list4_iter;
			gtk_list_store_append(store4, &list4_iter);
			gtk_list_store_set(store4, &list4_iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, row[2] ? row[2] : "", 3, pixbuf_xls, 4, pixbuf_csv,5, pixbuf_txt,-1);
			snprintf(id_relatorioADM,sizeof(id_relatorioADM),"%s",row[4]);
		}
		mysql_free_result(lista4);
	}
	char lista_mensagens_nao_lidas[250];
	char mensagens_nao_lidas[250];
	snprintf(lista_mensagens_nao_lidas,sizeof(lista_mensagens_nao_lidas),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_destinatario= '%s'",Nome);
	if(mysql_query(conexao,lista_mensagens_nao_lidas)==0)
	{
		mansagens_pendentes = mysql_store_result(conexao);
		if (mansagens_pendentes != NULL && mysql_num_rows(mansagens_pendentes)>0)
		{
			while(row = mysql_fetch_row(mansagens_pendentes))
			{
				snprintf(mensagens_nao_lidas,sizeof(mensagens_nao_lidas),"%s Mensagens Não Lidas",row[0]);
				gtk_label_set_text(label,mensagens_nao_lidas);
			}
		}
		mysql_free_result(mansagens_pendentes);
	}



	mysql_close(conexao);






	return TRUE;
}










//--------------------------------------------------------------------------------------------Buscar Endereço---------------------------------------------------------------------------------------------------------------------

void buscar_endereco_por_cep(GtkEntry *entry, gpointer user_data)
{
	gchar *cep = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CEP_empresa")));
	GtkEntry *estado = GTK_ENTRY(gtk_builder_get_object(user_data, "Estado"));
	GtkEntry *cidade = GTK_ENTRY(gtk_builder_get_object(user_data, "Cidade_Empresa"));
	GtkEntry *bairro = GTK_ENTRY(gtk_builder_get_object(user_data, "Bairro_empresa"));
	char *valor_alterado = str_replace(cep, "-", "");
	if (strlen(valor_alterado) == 8)
	{
		gtk_entry_set_text(estado, "");
		gtk_entry_set_text(cidade, "");
		gtk_entry_set_text(bairro, "");



		CURL *curl;
		CURLcode res;

		struct MemoryStruct chunk;
		chunk.memory = malloc(1);
		chunk.size = 0;

		char url[256];
		snprintf(url, sizeof(url), "https://viacep.com.br/ws/%s/json/", valor_alterado);

		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();

		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

			// Realiza a requisição
			res = curl_easy_perform(curl);

			if (res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() falhou: %s\n", curl_easy_strerror(res));
				free(chunk.memory); // Libera a memória alocada
				curl_easy_cleanup(curl);
				return; // Retorna da função em caso de erro
			}

			// Parsing do JSON para extrair os dados desejados
			cJSON *json = cJSON_Parse(chunk.memory);
			if (json == NULL)
			{
				fprintf(stderr, "Erro ao fazer parsing do JSON: %s\n", cJSON_GetErrorPtr());
				free(chunk.memory); // Libera a memória alocada
				curl_easy_cleanup(curl);
				return; // Retorna da função em caso de erro
			}

			// Extrair os valores de bairro, cidade e uf
			const cJSON *bairro_item = cJSON_GetObjectItemCaseSensitive(json, "bairro");
			const cJSON *cidade_item = cJSON_GetObjectItemCaseSensitive(json, "localidade");
			const cJSON *uf_item = cJSON_GetObjectItemCaseSensitive(json, "uf");

			if (cJSON_IsString(bairro_item) && (bairro_item->valuestring != NULL) &&
			        cJSON_IsString(cidade_item) && (cidade_item->valuestring != NULL) &&
			        cJSON_IsString(uf_item) && (uf_item->valuestring != NULL))
			{
				// Armazenando os valores em variáveis
				char bairro_val[50];
				char cidade_val[50];
				char uf_val[3];

				strncpy(bairro_val, bairro_item->valuestring, sizeof(bairro_val) - 1);
				strncpy(cidade_val, cidade_item->valuestring, sizeof(cidade_val) - 1);
				strncpy(uf_val, uf_item->valuestring, sizeof(uf_val) - 1);

				bairro_val[sizeof(bairro_val) - 1] = '\0';
				cidade_val[sizeof(cidade_val) - 1] = '\0';
				uf_val[sizeof(uf_val) - 1] = '\0';

				// Atualiza os valores em variáveis globais ou em campos de entrada
				strcpy(Bairro, bairro_val);
				strcpy(Estado, uf_val);
				strcpy(Cidade, cidade_val);

				// Define o texto nos campos de entrada do GTK
				gtk_entry_set_text(estado, Estado);
				gtk_entry_set_text(cidade, Cidade);
				gtk_entry_set_text(bairro, Bairro);

				printf("Bairro: %s\n", bairro_val);
				printf("Cidade: %s\n", cidade_val);
				printf("UF: %s\n", uf_val);
			}
			else
			{
				mensagem("Erro", "Cidade, Bairro e Estado não encontrados", 0, "");
			}
			cJSON_Delete(json);
			free(chunk.memory); // Libera a memória alocada para chunk.memory
		}
		else
		{
			fprintf(stderr, "Erro ao inicializar CURL.\n");
		}

		curl_global_cleanup();
	}
	else
	{
		fprintf(stderr, "CEP inválido.\n");
	}
}

//------------------------------------------------------------------------------------------------------||Criacao de Relatorio / Usuario--------------------------------------------------------------------------------------------

void Criar_Usuario()
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Criar_Usuario"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_queue_draw(dialogo_widget);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
}

void Criar_Relatorio()
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Enviar Relatório"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_queue_draw(dialogo_widget);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
}

//                                                                     efeito de clicke nos botoes


//------------------------------------------------------------------ Botao para Tela Cadastrar Usuario-------------------------------------------------------------------------------------
void on_Cadastrar_Usuario_clicked(GtkButton *button, gpointer user_data)
{
	Criar_Usuario();
}



//------------------------------------------------------------------ Botao para enviar dados de login-------------------------------------------------------------------------------------

void on_Enviar_clicked(GtkButton *button, gpointer user_data)
{
	// Inicialização
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Usuario_adm = NULL, *Usuario_empresa = NULL, *Usuario_Nome = NULL, *Valor_Colunas = NULL, *Nome_da_empresa_salva = NULL, *Consulta_Mensagens = NULL, *Total_Valores = NULL, *mensagens_pendentes = NULL;
	MYSQL_ROW row;

	GtkBuilder *builder = GTK_BUILDER(user_data);
	GtkTreeView *Relatorios_ADM = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Relatorios_ADM"));

	// Verifica campos de entrada
	const gchar *email = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(builder, "Input_Email")));
	const gchar *senha = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(builder, "Input_Senha")));
	if (email == NULL || senha == NULL || strlen(email) == 0 || strlen(senha) == 0)
	{
		mensagem("Erro", "Email ou Senha não podem estar vazios", 0, "erro");
	}

	strncpy(usuario_atual.email, email, sizeof(usuario_atual.email) - 1);
	strncpy(usuario_atual.senha, senha, sizeof(usuario_atual.senha) - 1);

	char query[500];
	snprintf(query, sizeof(query), "SELECT senha FROM usuarios_admins WHERE email='%s'", usuario_atual.email);
	char user[500];
	snprintf(user, sizeof(user), "SELECT senha_usuario_empresa FROM usuarios_empresas WHERE email_empresa='%s'", usuario_atual.email);

	// Verifica consulta para usuários da empresa
	if (mysql_query(conexao, user) == 0)
	{
		Usuario_empresa = mysql_store_result(conexao);
	}
	else
	{
		mensagem("Erro", "Erro na consulta SQL para empresa", 0, "erro");

	}

	// Verifica consulta para administradores
	if (mysql_query(conexao, query) == 0)
	{
		Usuario_adm = mysql_store_result(conexao);
	}
	else
	{
		mensagem("Erro", "Erro na consulta SQL para admin", 0, "erro");

	}

	// Lógica para usuários da empresa
	if (Usuario_empresa != NULL && mysql_num_rows(Usuario_empresa) > 0)
	{
		while ((row = mysql_fetch_row(Usuario_empresa)) != NULL)
		{
			strncpy(valor, row[0], sizeof(valor) - 1);
		}
		if (verify_password(senha, valor))
		{
			char Nome_da_empresa[250];
			snprintf(Nome_da_empresa, sizeof(Nome_da_empresa), "SELECT razao_social FROM usuarios_empresas WHERE email_empresa='%s'", usuario_atual.email);
			if (mysql_query(conexao, Nome_da_empresa) == 0)
			{
				Nome_da_empresa_salva = mysql_store_result(conexao);
				if (Nome_da_empresa_salva != NULL && mysql_num_rows(Nome_da_empresa_salva) > 0)
				{
					while ((row = mysql_fetch_row(Nome_da_empresa_salva)) != NULL)
					{
						strncpy(Nome, row[0], sizeof(Nome) - 1);
					}
				}
				else
				{
					mensagem("Erro", "Nenhuma empresa encontrada com esse email", 0, "erro");

				}
			}
			else
			{
				mensagem("Erro", "Erro ao buscar nome da empresa", 0, "erro");

			}

			char ColunaDeDados[700];
			GtkListStore *list1 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));
			GtkListStore *list2 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore2"));
			gtk_list_store_clear(list1);
			gtk_list_store_clear(list2);

			if (!list1)
			{
				mensagem("Erro", "Erro ao acessar ListStore", 0, "erro");
			}

			snprintf(ColunaDeDados, sizeof(ColunaDeDados), "SELECT nome_arquivo, data, id_relatorio FROM relatorio_valores WHERE empresa = '%s'", Nome);
			if (mysql_query(conexao, ColunaDeDados) == 0)
			{
				Valor_Colunas = mysql_store_result(conexao);
			}
			else
			{
				mensagem("Erro", "Erro ao buscar dados de relatorio_valores", 0, "erro");
			}

			if (Valor_Colunas != NULL && mysql_num_rows(Valor_Colunas) > 0)
			{
				GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
				GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
				GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);
				while ((row = mysql_fetch_row(Valor_Colunas)) != NULL)
				{
					GtkTreeIter iter;
					gtk_list_store_append(list1, &iter);
					gtk_list_store_set(list1, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, pixbuf_xls, 3, pixbuf_csv, 4, pixbuf_txt, -1);
					snprintf(id_relatorioEmpresa,sizeof(id_relatorioEmpresa),"%s",row[2]);
				}
			}



			char Caixa_Mensagens[1024];
			snprintf(Caixa_Mensagens, sizeof(Caixa_Mensagens), "SELECT titulo_mensagem, corpo_mensagem, usuario_criador, data_de_criacao, id_mensagem FROM mensagens WHERE empresa_destino = '%s' ORDER BY id_mensagem DESC ", Nome);
			if (mysql_query(conexao, Caixa_Mensagens) == 0)
			{
				Consulta_Mensagens = mysql_store_result(conexao);
				if (Consulta_Mensagens != NULL)
				{
					int x = 0;
					GtkTreeIter list2_iter;
					while ((row = mysql_fetch_row(Consulta_Mensagens)) != NULL)
					{
						if(x==0)
						{
							snprintf(id_mensagem,sizeof(id_mensagem),"%s",row[4]);
						}
						char titulo[700];
						snprintf(titulo, sizeof(titulo), "Cabeçalho : %s", row[0]);
						gtk_list_store_append(list2, &list2_iter);
						gtk_list_store_set(list2, &list2_iter, 0, titulo, -1);

						gtk_list_store_append(list2, &list2_iter);
						gtk_list_store_set(list2, &list2_iter, 0, row[1], -1);
						gtk_list_store_append(list2, &list2_iter);
						char criado[200];
						snprintf(criado, sizeof(criado), "Criado por: %s", row[2]);
						gtk_list_store_set(list2, &list2_iter, 0, criado, -1);

						char data[200];
						gtk_list_store_append(list2, &list2_iter);
						snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!", row[3]);
						gtk_list_store_set(list2, &list2_iter, 0, data, -1);

						char divisoria[800];
						gtk_list_store_append(list2, &list2_iter);
						snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
						gtk_list_store_set(list2, &list2_iter, 0, divisoria, -1);
						x++;
					}
				}
			}
			else
			{
				mensagem("Erro", "Erro ao buscar mensagens", 0, "erro");
			}
			GtkLabel *label = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagens_nao_lidas_empresa")));
			// Agende a função para ser chamada a cada 5000 milissegundos (5 segundos)
			timer_id = g_timeout_add_seconds(5, (GSourceFunc)atualizar_tabelas, (gpointer)label);

			char lista_mensagens_nao_lidas[250];
			char mensagens_nao_lidas[250];
			snprintf(lista_mensagens_nao_lidas,sizeof(lista_mensagens_nao_lidas),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_destinatario= '%s'",Nome);
			if(mysql_query(conexao,lista_mensagens_nao_lidas)==0)
			{
				mensagens_pendentes = mysql_store_result(conexao);
				if (mensagens_pendentes != NULL && mysql_num_rows(mensagens_pendentes)>0)
				{
					while(row = mysql_fetch_row(mensagens_pendentes))
					{
						snprintf(mensagens_nao_lidas,sizeof(mensagens_nao_lidas),"%s Mensagens Não Lidas",row[0]);
						gtk_label_set_text(label,mensagens_nao_lidas);
					}
				}
				mysql_free_result(mensagens_pendentes);
			}



			GtkLabel *Valor_Total_Acumulado = GTK_LABEL(gtk_builder_get_object(user_data, "Valor_Empresa_Total"));

			char valor_formatado[255];
			char Valor_de_Gastos[500];
			snprintf(Valor_de_Gastos, sizeof(Valor_de_Gastos), "SELECT SUM(valor_custo) FROM relatorio_valores WHERE empresa ='%s' ", Nome);
			if (mysql_query(conexao, Valor_de_Gastos) == 0)
			{
				Total_Valores = mysql_store_result(conexao);
				if (Total_Valores != NULL && mysql_num_rows(Total_Valores) > 0)
				{
					while ((row = mysql_fetch_row(Total_Valores)) != NULL)
					{
						char *valor_Total = row[0];
						if (valor_Total != NULL) // Verifica se valor_Total não é NULL
						{
							formatar_valor_em_reais(valor_Total, valor_formatado, sizeof(valor_formatado));
						}
						else
						{
							// Trate o caso onde o resultado é NULL
							snprintf(valor_formatado, sizeof(valor_formatado), "0,00"); // Ou outro valor padrão
						}
					}
					gtk_label_set_text(Valor_Total_Acumulado, valor_formatado);
				}
				else
				{
					gtk_label_set_text(Valor_Total_Acumulado, "0,00"); // Caso não haja resultados
				}
			}
			else
			{
				mensagem("Erro", "Erro na consulta ao banco de dados", 0, "erro");
				gtk_label_set_text(Valor_Total_Acumulado, "0,00"); // Define um valor padrão em caso de erro
			}

			// Limpa campos de entrada
			GtkWidget *apagar_email = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Input_Email"));
			gtk_entry_set_text(GTK_ENTRY(apagar_email), "");

			GtkWidget *apagar_senha = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Input_Senha"));
			gtk_entry_set_text(GTK_ENTRY(apagar_senha), "");

			char valor_escolhido[200];
			snprintf(valor_escolhido, sizeof(valor_escolhido), "Bem Vindo %s", Nome);

			GtkLabel *empresa_selecionada = GTK_LABEL(gtk_builder_get_object(user_data, "Bem_Vindo"));
			gtk_label_set_text(empresa_selecionada, valor_escolhido);

			mensagem("Bem Vindo", "Usuario Logado com Sucesso", 1, Nome);
			GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "Telas"));
			gtk_stack_set_visible_child_name(stack, "Pagina_User");
		}
		else
		{
			mensagem("Erro", "Senha Inválida", 0, "erro");
		}
	}

	else if (Usuario_adm != NULL && mysql_num_rows(Usuario_adm) > 0)
	{
		while ((row = mysql_fetch_row(Usuario_adm)) != NULL)
		{
			strncpy(valor, row[0], sizeof(valor) - 1);
		}

		if (verify_password(senha, valor))
		{
			char ColunaDeDados[700];
			GtkListStore *list4 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore4"));
			GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
			GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
			GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);
			if (!list4)
			{
				mensagem("Erro", "Erro ao acessar ListStore", 0, "erro");
			}

			snprintf(ColunaDeDados, sizeof(ColunaDeDados), "SELECT nome_arquivo, data, empresa,id_relatorio FROM relatorio_valores ");
			if (mysql_query(conexao, ColunaDeDados) == 0)
			{
				Valor_Colunas = mysql_store_result(conexao);
				if (Valor_Colunas != NULL && mysql_num_rows(Valor_Colunas) > 0)
				{
					while ((row = mysql_fetch_row(Valor_Colunas)) != NULL)
					{
						GtkTreeIter iter;
						gtk_list_store_append(list4, &iter);
						gtk_list_store_set(list4, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, row[2] ? row[2] : "", 3, pixbuf_xls, 4, pixbuf_csv, 5, pixbuf_txt, -1);
						snprintf(id_relatorioADM,sizeof(id_relatorioADM),"%s",row[3]);
					}
				}
				else
				{

				}
			}
			else
			{
				mensagem("Erro", "Erro ao executar a consulta em relatorio_valores", 0, "erro");
			}

			GtkLabel *Valor_Total_Acumulado = GTK_LABEL(gtk_builder_get_object(user_data, "Valor_Total_empresas_ADM"));

			char valor_formatado[255];
			char Valor_de_Gastos[500];
			snprintf(Valor_de_Gastos, sizeof(Valor_de_Gastos), "SELECT SUM(valor_custo) FROM relatorio_valores");
			if (mysql_query(conexao, Valor_de_Gastos) == 0)
			{
				Total_Valores = mysql_store_result(conexao);
				if (Total_Valores != NULL && mysql_num_rows(Total_Valores) > 0)
				{
					while ((row = mysql_fetch_row(Total_Valores)) != NULL)
					{
						char *valor_Total = row[0];
						if (valor_Total != NULL) // Verifica se valor_Total não é NULL
						{
							formatar_valor_em_reais(valor_Total, valor_formatado, sizeof(valor_formatado));
						}
						else
						{
							// Trate o caso onde o resultado é NULL
							snprintf(valor_formatado, sizeof(valor_formatado), "0,00"); // Ou outro valor padrão
						}
					}
					gtk_label_set_text(Valor_Total_Acumulado, valor_formatado);
				}
				else
				{
					gtk_label_set_text(Valor_Total_Acumulado, "0,00"); // Caso não haja resultados
				}
			}
			else
			{
				mensagem("Erro", "Erro na consulta ao banco de dados", 0, "erro");
				gtk_label_set_text(Valor_Total_Acumulado, "0,00"); // Define um valor padrão em caso de erro
			}

			GtkWidget *apagar_email = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Input_Email"));
			gtk_entry_set_text(GTK_ENTRY(apagar_email), "");

			GtkWidget *apagar_senha = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Input_Senha"));
			gtk_entry_set_text(GTK_ENTRY(apagar_senha), "");

			char Nome_da_empresa[250];
			snprintf(Nome_da_empresa, sizeof(Nome_da_empresa), "SELECT nome_usuario FROM usuarios_admins WHERE email='%s'", usuario_atual.email);
			if (mysql_query(conexao, Nome_da_empresa) == 0)
			{
				Nome_da_empresa_salva = mysql_store_result(conexao);
				if (Nome_da_empresa_salva != NULL && mysql_num_rows(Nome_da_empresa_salva) > 0)
				{
					while ((row = mysql_fetch_row(Nome_da_empresa_salva)) != NULL)
					{
						strncpy(Nome, row[0], sizeof(Nome) - 1);
					}
				}
				else
				{
					mensagem("Erro", "Nenhum usuário encontrado com esse email", 0, "erro");
				}
			}
			else
			{
				mensagem("Erro", "Erro ao executar a consulta para nome da empresa", 0, "erro");
			}

			GtkListStore *list3 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore3"));

			char Coluna[256];
			snprintf(Coluna, sizeof(Coluna), "SELECT titulo_chamado, corpo_chamado, nome_empresa, data_criacao,id_chamado FROM chamados ORDER BY id_chamado DESC");
			if (mysql_query(conexao, Coluna) == 0)
			{
				Valor_Colunas = mysql_store_result(conexao);
				if (Valor_Colunas != NULL)
				{
					int x = 0 ;
					while ((row = mysql_fetch_row(Valor_Colunas)) != NULL)
					{
						if(x == 0)
						{
							snprintf(id_chamado,sizeof(id_chamado),"%s",row[4]);
						}
						GtkTreeIter list2_iter;
						char titulo[700];
						snprintf(titulo, sizeof(titulo), "Cabeçalho : %s", row[0]);
						gtk_list_store_append(list3, &list2_iter);
						gtk_list_store_set(list3, &list2_iter, 0, titulo, -1);

						gtk_list_store_append(list3, &list2_iter);
						gtk_list_store_set(list3, &list2_iter, 0, row[1], -1);

						char empresa[200];
						gtk_list_store_append(list3, &list2_iter);
						snprintf(empresa, sizeof(empresa), "Empresa Referente: %s ", row[2]);
						gtk_list_store_set(list3, &list2_iter, 0, empresa, -1);

						char data[200];
						gtk_list_store_append(list3, &list2_iter);
						snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!", row[3]);
						gtk_list_store_set(list3, &list2_iter, 0, data, -1);

						char divisoria[800];
						gtk_list_store_append(list3, &list2_iter);
						snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
						gtk_list_store_set(list3, &list2_iter, 0, divisoria, -1);
						x++;
					}
				}
				else
				{
					mensagem("Erro", "Nenhum dado encontrado em chamados", 0, "erro");
				}
			}
			else
			{
				mensagem("Erro", "Erro ao executar a consulta em chamados", 0, "erro");
			}





			GtkLabel *label = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagens_não_lidas")));
			// Agende a função para ser chamada a cada 5000 milissegundos (5 segundos)
			timer_id = g_timeout_add_seconds(5, (GSourceFunc)atualizar_tabelas, (gpointer)label);


			char lista_mensagens_nao_lidas[250];
			char mensagens_nao_lidas[250];
			snprintf(lista_mensagens_nao_lidas,sizeof(lista_mensagens_nao_lidas),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_destinatario= '%s'",Nome);
			if(mysql_query(conexao,lista_mensagens_nao_lidas)==0)
			{
				mensagens_pendentes = mysql_store_result(conexao);
				if (mensagens_pendentes != NULL && mysql_num_rows(mensagens_pendentes)>0)
				{
					while(row = mysql_fetch_row(mensagens_pendentes))
					{
						snprintf(mensagens_nao_lidas,sizeof(mensagens_nao_lidas),"%s Mensagens Não Lidas",row[0]);
						gtk_label_set_text(label,mensagens_nao_lidas);
					}
				}
				mysql_free_result(mensagens_pendentes);
			}


			char valor_escolhido[200];
			snprintf(valor_escolhido, sizeof(valor_escolhido), "Bem Vindo %s", Nome);
			GtkLabel *empresa_selecionada = GTK_LABEL(gtk_builder_get_object(user_data, "Bem_Vindo1"));
			gtk_label_set_text(empresa_selecionada, valor_escolhido);

			mensagem("Bem Vindo", "Usuario Logado com Sucesso", 1, Nome);

			GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "Telas"));
			gtk_stack_set_visible_child_name(stack, "Pagina_Adm");
		}
		else
		{
			mensagem("Erro", "Senha Inválida", 0, "erro");
		}
	}
	else
	{
		mensagem("Erro", "Usuario Invalido", 0, "erro");
	}

// Liberação dos resultados
	if (Usuario_empresa)
		mysql_free_result(Usuario_empresa);
	if (Usuario_adm)
		mysql_free_result(Usuario_adm);
	if (Usuario_Nome)
		mysql_free_result(Usuario_Nome);
	if (Valor_Colunas)
		mysql_free_result(Valor_Colunas);
	if (Nome_da_empresa_salva)
		mysql_free_result(Nome_da_empresa_salva);
	if (Consulta_Mensagens)
		mysql_free_result(Consulta_Mensagens);
	if (Total_Valores)
		mysql_free_result(Total_Valores);

    mysql_close(conexao);
}

//----------------------------------------------------------------------Fim botao envia dados --------------------------------------------------------------------------------------------------------



//------------------------------------------------------------------------Caixa para Cadastro---------------------------------------------------------------------------------------------------------


void on_Cancelar_Envio_clicked(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Criar_Usuario"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);

}

gboolean on_Caixa_Desativa_activate(GtkSwitch *switch_button, gboolean state, gpointer user_data)
{
	GtkStack *stack = GTK_STACK(gtk_builder_get_object(user_data, "Telas_Cadastro"));
	if (state)
	{
		gtk_stack_set_visible_child_name(stack, "Pagina_Criacao_Usuario_Admin");
		GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Criar_Usuario"));
		GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
		gtk_widget_set_name(dialogo_widget, "Cria_Usuario_Adm");
		gtk_widget_queue_draw(dialogo_widget);
	}
	else
	{
		gtk_stack_set_visible_child_name(stack, "Criacao_de_Usuario_Empresa");
		GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Criar_Usuario"));
		GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
		gtk_widget_set_name(dialogo_widget, "Cria_Usuario");
		gtk_widget_queue_draw(dialogo_widget);
	}
}

void on_Enviar_Dados_Usuario_Empresa(GtkButton *button, gpointer user_data)
{
//------------------------------------------------------Variaveis com Valores do Input--------------------------------------------------------------------------------------------------------

	MYSQL *conexao = obterConexao();

	gchar *nome_usuario = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Usuario_empresa")));

	gchar *Senha = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Senha_Usuario_Empresa")));

	gchar *Email = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Email_Usuario_Empresa")));

	gchar *CNPJ =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CNPJ_Empresa")));

	gchar *razao_social =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Razao_Social")));

	gchar *nome_fantasia =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Fantasia")));

	gchar *telefone_empresa =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Telefone_Empresa")));

	gchar *rua_empresa =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Rua_Empresa")));

	gchar *numero_rua_empresa =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Numero_Rua_Empresa")));

	gchar *bairro_empresa =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Bairro_empresa")));

	gchar *cidade_empresa =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Cidade_Empresa")));

	gchar *estado =  gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Estado")));

	gchar *cep_empresa = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CEP_empresa")));



//--------------------------------------------------------------------------------\\-----------------------------------------------------------------------------------------------------------------
	gchar *inputs[13] =
	{
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Usuario_empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Senha_Usuario_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Email_Usuario_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CNPJ_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Razao_Social"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Fantasia"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Telefone_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Rua_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Numero_Rua_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Bairro_empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Cidade_Empresa"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Estado"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CEP_empresa")))
	};

//--------------------------------------------------------------------------------\\------------------------------------------------------------------------------------------------------------------

	char *cnpj_refeito = str_replace(CNPJ,".","");
	cnpj_refeito = str_replace(cnpj_refeito,"-","");
	cnpj_refeito = str_replace(cnpj_refeito,"/","");

	char escaped_values[13][256];
	for (int i = 0; i < 13; i++)
	{
		mysql_real_escape_string(conexao, escaped_values[i], inputs[i], strlen(inputs[i]));
	}

	char *cep_empresa_convertido=str_replace(cep_empresa,"-","");

	int i;
	int erro = 0;
	char *valores[13]= {nome_usuario,Senha,Email,cnpj_refeito,razao_social,nome_fantasia,telefone_empresa,rua_empresa,numero_rua_empresa,bairro_empresa,cidade_empresa,estado,cep_empresa_convertido};

	for (i = 0; i < 13; i++)
	{

		char *valors = valores[i];

		if(i == 1)
		{
			if(strlen(valors) < 6)
			{
				if(strlen(valors) == 0)
				{
					erro = i+1;
					break;
				}
				else
				{
					erro = 14;
					break;
				}


			}

		}

		if (valors == NULL || strlen(valors) == 0 || valors == "")
		{
			erro = i+1;
			break;
		}


	}

	if(erro == 0 || erro > 14)
	{
		if(string_somente_numeros(cnpj_refeito) && strlen(cnpj_refeito) == 14)
		{


			if(string_somente_numeros(cep_empresa_convertido) && strlen(cep_empresa_convertido) == 8)
			{

				if(strlen(estado) == 2 && !string_somente_numeros(estado))
				{
					if(strlen(telefone_empresa) > 12)
					{
						char *valores_banco[14]= {nome_usuario,Senha,razao_social,CNPJ,razao_social,nome_fantasia,telefone_empresa,rua_empresa,numero_rua_empresa,bairro_empresa,cidade_empresa,estado,cep_empresa_convertido,Email};
						char *valor_para_pesquisa_banco[14]= {"nome_usuario_empresa", "senha_usuario_empresa", "nome_da_empresa", "cnpj", "razao_social", "nome_fantasia", "telefone", "rua", "numero_empresa", "bairro", "cidade", "estado", "cep", "email_empresa"};
						int Semelhante = 0;
						int x=0;
						for(x=0; x < 14; x++)
						{
							char *values = valores_banco[x];
							char *chave = valor_para_pesquisa_banco[x];
							if(x==1 || x >= 7 && x < 13 )
							{

								continue;
							}
							char pesquisa[250];
							snprintf(pesquisa, sizeof(pesquisa), "SELECT * FROM usuarios_empresas WHERE %s = '%s'",chave,values);
							if (mysql_query(conexao, pesquisa))
							{
								continue;
							}
							MYSQL_RES *valor_final = mysql_store_result(conexao);
							if(valor_final == NULL )
							{

								continue;

							}
							if (mysql_num_rows(valor_final) > 0)
							{
								mysql_free_result(valor_final); // Libera o resultado antes de sair
								Semelhante = x+1;
								break;
							}
							mysql_free_result(valor_final);
						}
						if(Semelhante == 0 || Semelhante > 15)
						{


							char data_hora[20];

							// Cria uma variável para armazenar o tempo
							time_t tempo_atual;

							// Obtém o tempo atual
							time(&tempo_atual);

							// Converte para uma estrutura de tempo legível
							struct tm *data_hora_local = localtime(&tempo_atual);

							sprintf(data_hora, "%04d-%02d-%02d %02d:%02d:%02d",
							        data_hora_local->tm_year + 1900,  // Ano
							        data_hora_local->tm_mon + 1,      // Mês (começa em 0, por isso somamos 1)
							        data_hora_local->tm_mday,         // Dia
							        data_hora_local->tm_hour,         // Hora
							        data_hora_local->tm_min,          // Minuto
							        data_hora_local->tm_sec);         // Segundo


							char hashed_password[crypto_pwhash_STRBYTES];

							if (hash_user_password(Senha, hashed_password) == 0)
							{

							}


							char dados[1024];
							snprintf(dados, sizeof(dados), "INSERT INTO usuarios_empresas(nome_usuario_empresa, senha_usuario_empresa, nome_da_empresa, cnpj, razao_social, nome_fantasia, telefone, rua, numero_empresa, bairro, cidade, estado, cep, email_empresa, data_de_cadastro) VALUES('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
							         escaped_values[0], hashed_password, escaped_values[4], escaped_values[3], escaped_values[4], escaped_values[5], escaped_values[6], escaped_values[7], escaped_values[8], escaped_values[9], escaped_values[10], escaped_values[11], cep_empresa_convertido, escaped_values[2], data_hora);


							if (mysql_query(conexao, dados))
							{
								g_print("Erro na execução da query: %s\n", mysql_error(conexao));
							}
							char texto[50];
							snprintf(texto, sizeof(texto),"Usuário Criado : %s",nome_usuario);

							mensagem("Usuário Criado com Sucesso",texto,1,"");

							GtkWidget *nome_usuario_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Nome_Usuario_empresa"));
							gtk_entry_set_text(GTK_ENTRY(nome_usuario_apagar), "");

							GtkWidget *Senha_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Senha_Usuario_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(Senha_apagar), "");

							GtkWidget *email_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Email_Usuario_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(email_apagar), "");

							GtkWidget *Cnpj_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "CNPJ_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(Cnpj_apagar), "");

							GtkWidget *Razao_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Razao_Social"));
							gtk_entry_set_text(GTK_ENTRY(Razao_apagar), "");

							GtkWidget *Nome_fantasia_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Nome_Fantasia"));
							gtk_entry_set_text(GTK_ENTRY(Nome_fantasia_apagar), "");

							GtkWidget *Telefone_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Telefone_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(Telefone_apagar), "");

							GtkWidget *Rua_empresa_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Rua_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(Rua_empresa_apagar), "");

							GtkWidget *numero_empresa_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Numero_Rua_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(numero_empresa_apagar), "");

							GtkWidget *Bairro_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Bairro_empresa"));
							gtk_entry_set_text(GTK_ENTRY(Bairro_apagar), "");

							GtkWidget *Cidade_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Cidade_Empresa"));
							gtk_entry_set_text(GTK_ENTRY(Cidade_apagar), "");

							GtkWidget *Estado_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Estado"));
							gtk_entry_set_text(GTK_ENTRY(Estado_apagar), "");

							GtkWidget *CEP_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "CEP_empresa"));
							gtk_entry_set_text(GTK_ENTRY(CEP_apagar), "");
						}
						else
						{
							if(Semelhante == 1)
								mensagem("Erro de Formulario","Nome de Usuario já existe",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 3)
								mensagem("Erro de Formulario","Nome da Empresa já existe",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 4)
								mensagem("Erro de Formulario","Numero de CNPJ já existe",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 5)
								mensagem("Erro de Formulario","Razão Social já cadastrado",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 6)
								mensagem("Erro de Formulario","Nome Fantasia já cadastrado ",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 7)
								mensagem("Erro de Formulario","O telefone já existe",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
							if(Semelhante == 14)
								mensagem("Erro de Formulario","Email já existe",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");

						}

					}
					else
					{
						mensagem("Erro de Formulario","Telefone inserido incorretamente",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");

					}


				}
				else
				{
					mensagem("Erro de Formulario","Estado inserido incorretamente",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
				}




			}
			else
			{
				mensagem("Erro de Formulario","CEP inserido incorretamente",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
			}
		}
		else
		{
			mensagem("Erro de Formulario","CNPJ inserido incorretamente",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
		}

	}
	else
	{
		if(erro == 1)
			mensagem("Erro de Formulario","Insira um Nome",0,"Nome não foi Inserido");
		if(erro == 2)
			mensagem("Erro de Formulario","Insira uma Senha",0,"Senha não foi Inserido");
		if(erro == 3)
			mensagem("Erro de Formulario","Insira um Email",0,"Email não foi Inserido");
		if(erro == 4)
			mensagem("Erro de Formulario","Insira um CNPJ",0,"CNPJ não foi Inserido ou Apresenta erros de escrição");
		if(erro == 5)
			mensagem("Erro de Formulario","Insira uma Razão",0,"Razão Social não foi Inserido");
		if(erro == 6)
			mensagem("Erro de Formulario","Insira um Nome Fantasia",0,"Nome Fantasia não foi Inserido");
		if(erro == 7)
			mensagem("Erro de Formulario","Insira um Telefone",0,"Telefone para contato não foi Inserido");
		if(erro == 8)
			mensagem("Erro de Formulario","Insira uma Rua",0,"Rua não foi Inserido");
		if(erro == 9)
			mensagem("Erro de Formulario","Insira o Numero da Rua",0,"Numero da Rua não foi Inserido");
		if(erro == 10)
			mensagem("Erro de Formulario","Insira um Bairro",0,"Bairro não foi Inserido");
		if(erro == 11)
			mensagem("Erro de Formulario","Insira uma Cidade",0,"Cidade não foi Inserido");
		if(erro == 12)
			mensagem("Erro de Formulario","Insira um Estado",0,"Estado não foi Inserido");
		if(erro == 13)
			mensagem("Erro de Formulario","Insira um Cep",0,"Cep não foi Inserido");
		if(erro == 14)
			mensagem("Erro de Formulario","Insira uma Senha com mais de 5 caracteres",0,"Cep não foi Inserido");

	}

    mysql_close(conexao);

}

void on_Enviar_Dados_Usuario_ADM(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();

	gchar *nome_usuario_adm = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Adm")));
	gchar *nick_usuario = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nick_de_Usuario")));
	gchar *senha_de_usuario_adm = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Senha_de_Usuario_ADM")));
	gchar *cpf = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CPF_de_Usuario_ADM")));
	gchar *email_de_usuario_ADM = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Email_de_Usuario_ADM")));

//---------------------------------------------------------------------------------------\\--------------------------------------------------------------------------------------------------------------------------------------
	gchar *inputs[5] =
	{
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nome_Adm"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Nick_de_Usuario"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Senha_de_Usuario_ADM"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "CPF_de_Usuario_ADM"))),
		gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Email_de_Usuario_ADM")))
	};

//----------------------------------------------------------------------------------------\\--------------------------------------------------------------------------------------------------------------------------------
	char escaped_values[5][256];
	for (int i = 0; i < 5; i++)
	{
		mysql_real_escape_string(conexao, escaped_values[i], inputs[i], strlen(inputs[i]));
	}

	gchar *cpf_formatado = str_replace(cpf,".","");
	cpf_formatado = str_replace(cpf_formatado,"-","");

	int erro = 0;
	int i=0;
	char *valores[5] = {nome_usuario_adm,nick_usuario,senha_de_usuario_adm,cpf_formatado,email_de_usuario_ADM};

	for (i = 0; i < 5; i++)
	{

		char *valors = valores[i];

		if(i == 2)
		{
			if(strlen(valors) < 6)
			{
				if(strlen(valors) == 0)
				{
					erro = i+1;
					break;
				}
				else
				{
					erro = 6;
					break;
				}
			}
		}

		if(i == 3)
		{
			if(strlen(valors) <= 10)
			{


				if(strlen(valors) == 0)
				{
					erro = i+1;
					break;
				}
				else
				{
					erro = 7;
					break;
				}
			}
			else
			{
				if(!string_somente_numeros(valors))
				{
					erro = 8;
					break;
				}

			}
		}



		if (valors == NULL || strlen(valors) == 0 || valors == "")
		{
			erro = i+1;
			break;
		}

	}
	if(erro == 0 || erro > 8)
	{

		char *valores_bancos[5]= {nome_usuario_adm,nick_usuario,senha_de_usuario_adm,cpf,email_de_usuario_ADM};
		char *valor_para_pesquisa_bancos[5]= {"nome", "nome_usuario", "senha", "cpf", "email"};
		int Semelhante = 0;
		int x=0;
		for(x=0; x < 5; x++)
		{
			char *valuess = valores_bancos[x];
			char *chaves = valor_para_pesquisa_bancos[x];
			if(x==0 || x == 2)
			{
				continue;
			}
			char pesquisar[250];
			snprintf(pesquisar, sizeof(pesquisar), "SELECT * FROM usuarios_admins WHERE %s = '%s'",chaves,valuess);
			if (mysql_query(conexao, pesquisar))
			{
				continue;
			}
			MYSQL_RES *valor_finals = mysql_store_result(conexao);
			if(valor_finals == NULL )
			{

				continue;

			}
			if (mysql_num_rows(valor_finals) > 0)
			{
				mysql_free_result(valor_finals); // Libera o resultado antes de sair
				Semelhante = x+1;
				break;
			}
			mysql_free_result(valor_finals);
		}



		if(Semelhante == 0 || Semelhante > 6 )
		{

			char data_hora[20];

			// Cria uma variável para armazenar o tempo
			time_t tempo_atual;

			// Obtém o tempo atual
			time(&tempo_atual);

			// Converte para uma estrutura de tempo legível
			struct tm *data_hora_local = localtime(&tempo_atual);

			sprintf(data_hora, "%04d-%02d-%02d %02d:%02d:%02d",
			        data_hora_local->tm_year + 1900,  // Ano
			        data_hora_local->tm_mon + 1,      // Mês (começa em 0, por isso somamos 1)
			        data_hora_local->tm_mday,         // Dia
			        data_hora_local->tm_hour,         // Hora
			        data_hora_local->tm_min,          // Minuto
			        data_hora_local->tm_sec);         // Segundo

			char dados[750];

			char hashed_password[crypto_pwhash_STRBYTES];

			if (hash_user_password(senha_de_usuario_adm, hashed_password) == 0)
			{

			}


			snprintf(dados, sizeof(dados), "INSERT INTO usuarios_admins(nome, nome_usuario, senha, cpf, email, data_de_criacao, usuario_criador) VALUES('%s', '%s', '%s', '%s', '%s', '%s', '%s')",
			         escaped_values[0], escaped_values[1], hashed_password, cpf_formatado, escaped_values[4], data_hora, Nome);


			if (mysql_query(conexao, dados))
			{
				g_print("Erro na execução da query: %s\n", mysql_error(conexao));
			}
			char texto[50];
			snprintf(texto, sizeof(texto),"Usuário Criado : %s",nome_usuario_adm);

			mensagem("Usuário Criado com Sucesso",texto,1,"");

			GtkWidget *nome_usuario_adm_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Nome_Adm"));
			gtk_entry_set_text(GTK_ENTRY(nome_usuario_adm_apagar), "");

			GtkWidget *nick_usuario_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Nick_de_Usuario"));
			gtk_entry_set_text(GTK_ENTRY(nick_usuario_apagar), "");

			GtkWidget *senha_de_usuario_adm_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Senha_de_Usuario_ADM"));
			gtk_entry_set_text(GTK_ENTRY(senha_de_usuario_adm_apagar), "");

			GtkWidget *cpf_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "CPF_de_Usuario_ADM"));
			gtk_entry_set_text(GTK_ENTRY(cpf_apagar), "");

			GtkWidget *email_de_usuario_ADM_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Email_de_Usuario_ADM"));
			gtk_entry_set_text(GTK_ENTRY(email_de_usuario_ADM_apagar), "");

		}
		else
		{
			if(Semelhante == 2)
				mensagem("Erro de Formulario","Nick de Usuario já existe",0,"");
			if(Semelhante == 4)
				mensagem("Erro de Formulario","CPF de Usuario já existe",0,"");
			if(Semelhante == 5)
				mensagem("Erro de Formulario","Email de Usuario já existe",0,"");
		}




	}
	else
	{
		if(erro==1)
		{
			mensagem("Erro de Formulario","Insira um Nome",0,"Nome não foi Inserido");

		}
		if(erro == 2)
		{
			mensagem("Erro de Formulario","Insira um Nick de Usuario",0,"Nome não foi Inserido");
		}
		if(erro == 3)
		{
			mensagem("Erro de Formulario","Insira uma Senha",0,"Nome não foi Inserido");
		}
		if(erro == 4)
			mensagem("Erro de Formulario","Insira um CPF",0,"Nome não foi Inserido");
		if(erro == 5)
		{
			mensagem("Erro de Formulario","Insira um Email",0,"Nome não foi Inserido");

		}
		if(erro == 6)
			mensagem("Erro de Formulario","Insira uma Senha com mais de 5 caracteres",0,"Cep não foi Inserido");

		if(erro == 7)
			mensagem("Erro de Formulario","Insira uma CPF com mais de 11 caracteres",0,"Cep não foi Inserido");

		if(erro == 8)
			mensagem("Erro de Formulario","Insira um CPF válido",0,"Cep não foi Inserido");

	}
mysql_close(conexao);
}



//---------------------------------------------------------------------------Fim cadastro de Usuario-------------------------------------------------------------------------------------------------------------



void on_botao_sair_caixa_mensagem(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Erro_Logout"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

// Callback para o sinal destroy
void on_Janela_Principal_destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit(); // Fecha o aplicativo
}


// Callback para o bot�o Login
void on_Botao_Login_clicked(GtkButton *button, gpointer user_data)
{
	GtkStack *stack = GTK_STACK(user_data);  // Cast para GtkStack
	gtk_stack_set_visible_child_name(stack, "Pagina_Login");  // Troca para a p�gina de login
}
void on_Login_clicked(GtkWidget *Login, GdkEventButton *event, gpointer user_data)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		GtkStack *stack = GTK_STACK(user_data);  // Cast para GtkStack
		gtk_stack_set_visible_child_name(stack, "Pagina_Login");  // Troca para a p�gina de login
	}
}

// Callback para o bot�o Logout
void on_BotaoVoltar_clicked(GtkButton *button, gpointer user_data)
{
	GtkStack *stack = GTK_STACK(user_data);  // Cast para GtkStack
	gtk_stack_set_visible_child_name(stack, "Iniciar");  // Troca para a p�gina "Iniciar"
}



// --------------------------------------------------------------------------Inicio do Bot�o Efetuar Logout-----------------------------------------------------------------------------------------------



void on_Efetuar_Logout_clicked(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	GtkBuilder *builder = GTK_BUILDER(user_data);
	GtkStack *janela = GTK_STACK(gtk_builder_get_object(builder, "Telas"));
	GtkListStore *list1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore1"));
	GtkListStore *list4 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore4"));
	GtkListStore *list2 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore2"));
	GtkListStore *list3 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore3"));
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore8"));
	GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
	gtk_list_store_clear(list4);
	gtk_list_store_clear(list1);
	gtk_list_store_clear(list2);
	gtk_list_store_clear(list3);
	gtk_list_store_clear(list8);
	gtk_list_store_clear(list10);
	g_source_remove(timer_id);
	timer_id = 0;
	snprintf(hora_mensagem,sizeof(hora_mensagem),"0");
	snprintf(id_chamado,sizeof(id_chamado),"0");
	snprintf(id_mensagem,sizeof(id_mensagem),"0");
	snprintf(id_relatorioEmpresa,sizeof(id_relatorioEmpresa),"0");
	snprintf(id_relatorioADM,sizeof(id_relatorioADM),"0");

	gtk_stack_set_visible_child_name(janela, "Iniciar");  // Troca para a p�gina "Iniciar"
	memset(usuario_atual.email, 0, sizeof(usuario_atual.email));
	memset(usuario_atual.senha, 0, sizeof(usuario_atual.senha));
	mysql_close(conexao);
}




// --------------------------------------------------------------------------------Fim do Bot�o Efetuar Logout-----------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------Caixa de Geracao de Relatorios------------------------------------------------------------------------------------------------------//

void puxar_dados(gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Nomes_Empresas;
	MYSQL_ROW row;
	char Razao_empresas[250];

	GtkListStore *list5 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore5"));
	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Nomes_Empresas = mysql_store_result(conexao);
	if(Nomes_Empresas == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
	{
		GtkTreeIter iter;
		gtk_list_store_append(list5, &iter);
		gtk_list_store_set(list5, &iter, 0, row[0],-1);
	}
    mysql_close(conexao);
}
void puxar_dados_selecao(gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Nomes_Empresas;
	MYSQL_ROW row;
	char Razao_empresas[250];

	GtkListStore *list7 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Nomes_Empresas = mysql_store_result(conexao);
	if(Nomes_Empresas == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
	{
		GtkTreeIter iter;
		gtk_list_store_append(list7, &iter);
		gtk_list_store_set(list7, &iter, 0, row[0],-1);
	}
    mysql_close(conexao);
}



void on_Gerar_Relatorio(GtkButton *button, gpointer user_data)
{

	Criar_Relatorio();


}



void on_Cancelar_Relatorio(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Enviar Relatório"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);


}

void on_selecionar_empresa(GtkButton *button, gpointer user_data)
{

	puxar_dados(user_data);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"selecionar_empresa"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_queue_draw(dialogo_widget);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
	GtkListStore *list5 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore5"));
	gtk_list_store_clear(list5);




}
void on_pesquisar_relatorio_entry_changed(GtkEntry *entry, gpointer user_data)
{
	char *search_text = gtk_entry_get_text(entry);
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Nomes_Empresas;
	MYSQL_ROW row;
	GtkListStore *list5 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore5"));

	GtkTreeIter iter;

	if(search_text == NULL || search_text == "")
	{
		char Razao_empresas[250];
		gtk_list_store_clear(list5);

		snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas");

		if(mysql_query(conexao,Razao_empresas))
		{
			g_print("Deu ruim");
		}
		Nomes_Empresas = mysql_store_result(conexao);
		if(Nomes_Empresas == NULL)
		{
			g_print("Deu ruim mais ainda");
		}
		while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(list5, &iter);
			gtk_list_store_set(list5, &iter, 0, row[0],-1);
		}
	}
	else
	{
		gtk_list_store_clear(list5);
		char Razao_empresas[250];
		GtkListStore *list5 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore5"));
		snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas WHERE razao_social LIKE '%%%s%%'",search_text);

		if(mysql_query(conexao,Razao_empresas))
		{
			g_print("Deu ruim");
		}
		Nomes_Empresas = mysql_store_result(conexao);
		if(Nomes_Empresas == NULL)
		{
			g_print("Deu ruim mais ainda");
		}
		while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(list5, &iter);
			gtk_list_store_set(list5, &iter, 0, row[0],-1);
		}
	}


	mysql_close(conexao);

}
void on_row_activated(GtkTreeView *listaEmpresas, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkListStore *store = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore5"));
	GtkTreeModel *model = gtk_tree_view_get_model(listaEmpresas);
	GtkTreeIter iter;

	// Obtém o iterador da linha ativada
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gchar *nome_da_empresa;


		gtk_tree_model_get(model, &iter, 0, &nome_da_empresa,-1);
		char valor_escolhido[200];
		snprintf(valor_escolhido,sizeof(valor_escolhido),"%s",nome_da_empresa);
		GtkLabel *empresa_selecionada = GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa_Relatorio"));
		gtk_label_set_text(empresa_selecionada,valor_escolhido);

	}

	gtk_list_store_clear(store);

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"selecionar_empresa"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

//-----------------------------------------------------------------------------------Envento de clique para enviar relatorio--------------------------------------------------------------------------------------------------------

void on_enviar_relatorio(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();

	MYSQL_RES *envio_relatorios = NULL, *Valor_Colunas =NULL;
	MYSQL_ROW row;
	char chave_completa[5] = "";
	char data_hora[20];
	char valor_arquivo[250] = "";
	char nome_arquivo_padrao[100] = "";
	int erro = 0;

	time_t tempo_atual;
	time(&tempo_atual);


	struct tm *data_hora_local = localtime(&tempo_atual);
	snprintf(data_hora, sizeof(data_hora), "%04d-%02d-%02d %02d:%02d:%02d",
	         data_hora_local->tm_year + 1900, data_hora_local->tm_mon + 1,
	         data_hora_local->tm_mday, data_hora_local->tm_hour,
	         data_hora_local->tm_min, data_hora_local->tm_sec);

	GtkLabel *empresa_selecionada = GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa_Relatorio"));
	const char *label_nome_empresa = gtk_label_get_text(empresa_selecionada);

	gchar *quantidade_residuo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Residuos")));
	gchar *valor_custo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Valor_Custo")));
	gchar *nome_arquivo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "nome_arquivo")));

	// Formata o valor, removendo "R$" e outros caracteres
	char *valor_formatado = str_replace(valor_custo, "R$", "");
	valor_formatado = str_replace(valor_formatado, ".", "");
	valor_formatado = str_replace(valor_formatado, ",", ".");
	char *valor_final = str_replace(valor_formatado,".","");
	// Define nome_arquivo com um valor padrão se estiver vazio
	if (nome_arquivo == NULL || strcmp(nome_arquivo, "") == 0)
	{
		snprintf(nome_arquivo_padrao, sizeof(nome_arquivo_padrao), "Relatorio_Criado_em_%s_por_%s", data_hora, Nome);
	}
	else
	{
		snprintf(nome_arquivo_padrao, sizeof(nome_arquivo_padrao), "%s_Criado_em_%s_por_%s", nome_arquivo, data_hora, Nome);
	}

	// Checa se há algum valor vazio, exceto para nome_arquivo
	char *valores_vazios[3] = {label_nome_empresa, quantidade_residuo, valor_formatado};
	for (int i = 0; i < 3; i++)
	{
		if (valores_vazios[i] == NULL || strcmp(valores_vazios[i], "") == 0)
		{
			erro = 1;
			break;
		}
	}

	if (erro)
	{
		mensagem("Erro de Formulário", "Há valores necessários não preenchidos.", 0, "");
	}

	if (string_somente_numeros(valor_final))
	{
		char chave[200];
		snprintf(chave, sizeof(chave), "SELECT id_usuario_empresa FROM usuarios_empresas WHERE razao_social = '%s'", label_nome_empresa);

		if (mysql_query(conexao, chave) != 0)
		{
			mensagem("Erro de Consulta", "Erro ao executar a consulta de empresa.", 0, mysql_error(conexao));

		}

		envio_relatorios = mysql_store_result(conexao);
		if (envio_relatorios)
		{
			row = mysql_fetch_row(envio_relatorios);
			if (row && row[0])
			{
				strncpy(chave_completa, row[0], sizeof(chave_completa) - 1);
			}

		}
		else
		{
			mensagem("Erro de Consulta", "Resultado da consulta está vazio.", 0, "");

		}
		mysql_free_result(envio_relatorios);

		if (strlen(chave_completa) > 0)
		{
			char envio_relatorio[500];
			snprintf(envio_relatorio, sizeof(envio_relatorio), "INSERT INTO relatorio_valores(nome_arquivo, chave, quantidade_residuos, valor_custo, empresa,data) VALUES ('%s', '%s', '%s', '%s', '%s','%s')",
			         nome_arquivo_padrao, chave_completa, quantidade_residuo, valor_formatado, label_nome_empresa,data_hora);

			if (mysql_query(conexao, envio_relatorio) != 0)
			{
				mensagem("Erro ao Inserir", "Falha ao inserir relatório no banco de dados.", 0, mysql_error(conexao));


			}

			char valor_necessario[200];
			snprintf(valor_necessario, sizeof(valor_necessario), "SELECT id_relatorio FROM relatorio_valores WHERE nome_arquivo = '%s'", nome_arquivo_padrao);

			if (mysql_query(conexao, valor_necessario) != 0)
			{
				mensagem("Erro ao Recuperar", "Erro ao buscar ID do relatório.", 0, mysql_error(conexao));

				return;
			}

			envio_relatorios = mysql_store_result(conexao);
			if (envio_relatorios != NULL && mysql_num_rows(envio_relatorios)>0)
			{
				row = mysql_fetch_row(envio_relatorios);
				if (row && row[0])
				{
					strncpy(id_relatorioADM, row[0], sizeof(id_relatorioADM) - 1);
				}

				char ColunaDeDados[700];
				GtkListStore *list4 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore4"));
                GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
                GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
                GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);
				if (!list4)
				{
					mensagem("Erro", "Erro ao acessar ListStore", 0, "erro");
				}

				snprintf(ColunaDeDados, sizeof(ColunaDeDados), "SELECT nome_arquivo, data, empresa, id_relatorio FROM relatorio_valores WHERE id_relatorio = %s",id_relatorioADM);
				if (mysql_query(conexao, ColunaDeDados) == 0)
				{
					Valor_Colunas = mysql_store_result(conexao);
				}
				if (Valor_Colunas != NULL)
				{
					while ((row = mysql_fetch_row(Valor_Colunas)) != NULL)
					{
						GtkTreeIter iter;
						gtk_list_store_append(list4, &iter);
						gtk_list_store_set(list4, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, row[2] ? row[2] : "", 3, pixbuf_xls, 4, pixbuf_csv,5, pixbuf_txt,-1);
						snprintf(id_relatorioADM,sizeof(id_relatorioADM),"%s",row[3]);
					}
				}


				mysql_free_result(Valor_Colunas);
				mysql_free_result(envio_relatorios);
				char token[256] = {0};
				obter_token(token,sizeof(token));
				char dados_usuario[256];

				snprintf(dados_usuario, sizeof(dados_usuario),"%s*Quantidade residuos; Valor de custo; : %s; %s;",id_relatorioADM,quantidade_residuo,valor_custo);
				enviar_dados_usuario(token,dados_usuario);

				mensagem("Relatório Enviado com Sucesso", "Dados Adicionados e enviados", 1, "");
				GtkWidget *Apagar_residuos = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Residuos"));
				gtk_entry_set_text(GTK_ENTRY(Apagar_residuos), "");

				GtkWidget *Apagar_nome_arquivo = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "nome_arquivo"));
				gtk_entry_set_text(GTK_ENTRY(Apagar_nome_arquivo), "");

				GtkWidget *apagar_custo = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Valor_Custo"));
				gtk_entry_set_text(GTK_ENTRY(apagar_custo), "");

				gtk_label_set_text(empresa_selecionada,"");
			}
		}
	}
	else
	{
		mensagem("Erro de Formulário", "Há caracteres irregulares no valor de custo.", 0, "");
	}

	mysql_close(conexao);
}


// Função para gerar CSV a partir do GtkListStore em um local específico
void gerar_csv_do_liststore(GtkButton *button, gpointer user_data, char *caminho_completo)
{
	GtkListStore *list9 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore9"));
	GtkTreeIter iter;
	gboolean has_data = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list9), &iter);

	time_t agora = time(NULL);
	struct tm *tm_info = localtime(&agora);
	char data_hora[20];

	// Formata a data e hora como "YYYY_MM_DD_HH_MM_SS"
	strftime(data_hora, sizeof(data_hora), "%Y_%m_%d_%H_%M_%S", tm_info);
	char Arquivo_Relatorio_Geral[500];
	// Define o nome do arquivo com o caminho e data/hora formatada
	snprintf(Arquivo_Relatorio_Geral, sizeof(Arquivo_Relatorio_Geral), "%s/Relatorio_Geral_%s.csv",caminho_completo,data_hora);


	// Tenta abrir o arquivo para escrita no destino especificado
	FILE *arquivo_csv = fopen(Arquivo_Relatorio_Geral, "w");
	if (!arquivo_csv)
	{
		mensagem("Erro ao Criar o CSV","Tente Novamente",0,"");
		return;
	}

	// Escreve o cabeçalho do CSV
	fprintf(arquivo_csv, "Nome_Empresa;Quantidade de Insumos Tratados;Valor de Custo;Data\n");

	// Itera pelo liststore e escreve os dados no arquivo
	while (has_data)
	{
		gchar *quantidade_insumos;
		gchar *valor_custo;
		gchar *data;
		gchar *nome_empresa;

		gtk_tree_model_get(GTK_TREE_MODEL(list9), &iter,
		                   1, &nome_empresa,
		                   2, &quantidade_insumos,
		                   3, &valor_custo,
		                   4, &data,
		                   -1);

		fprintf(arquivo_csv, "%s;%s;%s;%s\n",nome_empresa, quantidade_insumos, valor_custo, data);

		g_free(nome_empresa);
		g_free(quantidade_insumos);
		g_free(valor_custo);
		g_free(data);

		has_data = gtk_tree_model_iter_next(GTK_TREE_MODEL(list9), &iter);
	}

	fclose(arquivo_csv);
    char Caminhodoarquivo[400];
    snprintf(Caminhodoarquivo,sizeof(Caminhodoarquivo),"Baixado em %s",caminho_completo);
    mensagem("Dowload Comcluido com Sucesso",Caminhodoarquivo,1,"");
}



//-----------------------------------------------------------------------------------Evento para escolher pasta de Dowload------------------------------------------------------------------------------------------------------------
void on_Selecionar_pasta_clicked(GtkButton *button, gpointer user_data)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(user_data);
	char *folder_path = gtk_file_chooser_get_filename(chooser);

	if (folder_path)
	{
		if(geral == 0)
		{
			char token[256] = {0};
			obter_token(token,sizeof(token));
			baixar_arquivo(token,Arquivo,Tipo,folder_path);
			g_free(folder_path);
		}
		else if(geral == 1)
		{
			gerar_csv_do_liststore(button, user_data, folder_path);
		}
	}

	gtk_widget_hide(GTK_WIDGET(chooser)); // Fecha o diálogo após selecionar
}
void on_Cancelar_Busca_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(user_data)); // Fecha o diálogo
}

void on_open_file_chooser(gpointer user_data)
{
	GtkWidget *file_chooser_dialog = GTK_WIDGET(gtk_builder_get_object(user_data,"Escolher_local_para_dowload"));
	gtk_widget_show_all(GTK_WIDGET(file_chooser_dialog));
	gtk_dialog_run(GTK_DIALOG(file_chooser_dialog));
	gtk_widget_hide(file_chooser_dialog);


}



//-----------------------------------------------------------------------------------------Ativador de Dowload Arquivo--------------------------------------------------------------------------------------------------------------

void on_Relatorios_ADM_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gchar *nome;
		gtk_tree_model_get(model, &iter, 0, &nome, -1);

		if (g_strcmp0(gtk_tree_view_column_get_title(column), "TXT") == 0)
		{

			char token[256] = {0};
			char Tipo_Arquivo[100] = "txt";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);
		}

		else if (g_strcmp0(gtk_tree_view_column_get_title(column), "XLS") == 0)
		{
			char token[256] = {0};
			char Tipo_Arquivo[100] = "xls";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);

		}

		else if(g_strcmp0(gtk_tree_view_column_get_title(column), "CSV")==0)
		{

			char token[256] = {0};
			char Tipo_Arquivo[100] = "csv";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);
		}
		g_free(nome);
	}
}

void on_Tabelo_empresa_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gchar *nome;
		gtk_tree_model_get(model, &iter, 0, &nome, -1);

		if (g_strcmp0(gtk_tree_view_column_get_title(column), "TXT") == 0)
		{

			char token[256] = {0};
			char Tipo_Arquivo[100] = "txt";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);
		}

		else if (g_strcmp0(gtk_tree_view_column_get_title(column), "XLS") == 0)
		{

			char token[256] = {0};
			char Tipo_Arquivo[100] = "xls";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);

		}

		else if(g_strcmp0(gtk_tree_view_column_get_title(column), "CSV")==0)
		{


			char token[256] = {0};
			char Tipo_Arquivo[100] = "csv";

			strncpy(Tipo, Tipo_Arquivo, sizeof(Tipo) - 1);
			snprintf(Arquivo, sizeof(Arquivo), "%s", nome);
			geral = 0;
			on_open_file_chooser(user_data);
		}
		g_free(nome);
	}
}



//----------------------------------------------------------------------------------Envio de Mensagens ADM----------------------------------------------------------------------------------------------------------------------
void on_abrir_caixa_mensagem(GtkButton *button, gpointer user_data)
{


	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Caixa_de_Mensagem"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_queue_draw(dialogo_widget);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);




}

void on_sair_caixa_de_mensagem(GtkButton *button, gpointer user_data)
{
	GtkListStore *store = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	gtk_list_store_clear(store);
	gtk_list_store_clear(store1);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"Caixa_de_Mensagem"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
	GtkWidget *mensagem_titulo_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_da_mensagem"));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_da_mensagem"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	gtk_entry_set_text(GTK_ENTRY(mensagem_titulo_apagar), "");
	gtk_text_buffer_set_text(buffer, "", -1);

}

void on_Abrir_Selecao_de_empresas(GtkButton *button, gpointer user_data)
{
	puxar_dados_selecao(user_data);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Selecionar_empresas_mensagem"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_queue_draw(dialogo_widget);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	gtk_list_store_clear(store1);

}
void on_Selecionar_Todas(GtkToggleButton *toggle_button, gpointer user_data)
{
	gboolean is_active = gtk_toggle_button_get_active(toggle_button);
	if (is_active)
	{
		GtkListStore *lista6 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));

		gtk_list_store_clear(lista6);
		MYSQL *conexao = obterConexao();
		MYSQL_RES *Nomes_Empresas;
		MYSQL_ROW row;

		GtkTreeIter iter;

		char Razao_empresas[250];

		snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas");

		if(mysql_query(conexao,Razao_empresas))
		{
			g_print("Deu ruim");
		}
		Nomes_Empresas = mysql_store_result(conexao);
		if(Nomes_Empresas == NULL)
		{
			g_print("Deu ruim mais ainda");
		}
		while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(lista6, &iter);
			gtk_list_store_set(lista6, &iter, 0, row[0],-1);
		}
		mysql_free_result(Nomes_Empresas);
	}
	else
	{
		GtkListStore *lista6 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
		gtk_list_store_clear(lista6);
	}

}

void on_Enviar_Selecionados(GtkButton *button, gpointer user_data)
{

	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	gtk_list_store_clear(store1);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Selecionar_empresas_mensagem"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);

}

void on_Fechar_Selecao(GtkButton *button, gpointer user_data)
{
	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	gtk_list_store_clear(store1);

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Selecionar_empresas_mensagem"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);

}

void on_Apagar_Selecionados(GtkButton *button, gpointer user_data)
{

	GtkListStore *store1 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
	gtk_list_store_clear(store1);

}

void on_Retira_selecionado_activated(GtkTreeView *Retira_selecionado, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkListStore *list6 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
	GtkTreeModel *model = gtk_tree_view_get_model(Retira_selecionado);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_list_store_remove(list6, &iter);

	}
}

void on_Clique_Seleciona_activated(GtkTreeView *Clique_Seleciona, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkListStore *list7 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
	GtkListStore *list6 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
	GtkTreeView *Retira_selecionado = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Clique_Apaga")) ;
	// Verifica se list6 e list7 existem
	if (!list7 || !list6)
	{
		g_warning("Liststores não foram encontrados!");
		return;
	}

	GtkTreeModel *model = gtk_tree_view_get_model(Clique_Seleciona);
	GtkTreeIter iter;
	GtkTreeModel *Add_Dados = gtk_tree_view_get_model(Retira_selecionado);
	GtkTreeIter new_iter;
	// Obtém o iterador da linha ativada
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gchar *nome_da_empresa = NULL;
		gtk_tree_model_get(model, &iter, 0, &nome_da_empresa, -1);
		int erro = 1;
		if (nome_da_empresa)
		{
			gchar *valordados;
			if (gtk_tree_model_get_iter_first(Add_Dados, &new_iter))
			{
				do
				{
					// Obtém o valor da primeira coluna
					gtk_tree_model_get(Add_Dados,&new_iter, 0, &valordados, -1);

					if(g_strcmp0(nome_da_empresa, valordados) == 0)
					{
						erro = 0;
						break;

					}

					g_free(valordados);
				}
				while (gtk_tree_model_iter_next(Add_Dados, &new_iter));
			}
			if(erro == 1)
			{
				char valor_escolhido[200];
				snprintf(valor_escolhido, sizeof(valor_escolhido), "%s", nome_da_empresa);
				gtk_list_store_append(list6, &new_iter);
				gtk_list_store_set(list6, &new_iter, 0, valor_escolhido, -1);
			}
			else
			{
				mensagem("Erro","Empresa já Selecionada",0,"Erro");
			}

			g_free(nome_da_empresa);
		}
	}
}

void on_search_entry_changed(GtkEntry *entry, gpointer user_data)
{
	char *search_text = gtk_entry_get_text(entry);
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Nomes_Empresas;
	MYSQL_ROW row;
	GtkListStore *list7 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));

	GtkTreeIter iter;

	if(search_text == NULL || search_text == "")
	{
		char Razao_empresas[250];
		gtk_list_store_clear(list7);

		snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas");

		if(mysql_query(conexao,Razao_empresas))
		{
			g_print("Deu ruim");
		}
		Nomes_Empresas = mysql_store_result(conexao);
		if(Nomes_Empresas == NULL)
		{
			g_print("Deu ruim mais ainda");
		}
		while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(list7, &iter);
			gtk_list_store_set(list7, &iter, 0, row[0],-1);
		}
	}
	else
	{
		gtk_list_store_clear(list7);
		char Razao_empresas[250];
		GtkListStore *list7 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore7"));
		snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT razao_social FROM usuarios_empresas WHERE razao_social LIKE '%%%s%%'",search_text);

		if(mysql_query(conexao,Razao_empresas))
		{
			g_print("Deu ruim");
		}
		Nomes_Empresas = mysql_store_result(conexao);
		if(Nomes_Empresas == NULL)
		{
			g_print("Deu ruim mais ainda");
		}
		while((row = mysql_fetch_row(Nomes_Empresas)) != NULL)
		{
			GtkTreeIter iter;
			gtk_list_store_append(list7, &iter);
			gtk_list_store_set(list7, &iter, 0, row[0],-1);
		}
	}


	mysql_close(conexao);
}


void on_Enviar_Dados_Mensagem(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);

	struct tm *data_hora_local = localtime(&tempo_atual);
	snprintf(data_hora, sizeof(data_hora), "%04d-%02d-%02d %02d:%02d:%02d",
	         data_hora_local->tm_year + 1900, data_hora_local->tm_mon + 1,
	         data_hora_local->tm_mday, data_hora_local->tm_hour,
	         data_hora_local->tm_min, data_hora_local->tm_sec);

	gchar *Titulo_Mensagem = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_da_mensagem")));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_da_mensagem"));

	GtkListStore *lista_de_selecionados = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore6"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	GtkTextIter start, end;
	gchar *texto;

	// Obter o texto entre os iteradores
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	texto = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	// Verificar se o título ou o texto estão vazios
	if (g_strcmp0(Titulo_Mensagem, "") == 0)
	{
		mensagem("Erro", "Insira um Título para a Mensagem", 0, "Erro");
		g_free(texto);
		return;
	}
	if (g_strcmp0(texto, "") == 0)
	{
		mensagem("Erro", "Insira a Mensagem", 0, "Erro");
		g_free(texto);
		return;
	}

	// Preparar buffers de escape
	char titulo_escapado[512];
	char texto_escapado[512];
	mysql_real_escape_string(conexao, titulo_escapado, Titulo_Mensagem, strlen(Titulo_Mensagem));
	mysql_real_escape_string(conexao, texto_escapado, texto, strlen(texto));

	// Iterar pelos destinatários
	GtkTreeIter iterador;
	GtkTreeModel *Valores_Destinatarios = GTK_TREE_MODEL(lista_de_selecionados);
	GPtrArray *array_de_empresas = g_ptr_array_new_with_free_func(g_free);
	gchar *valor_linha;

	if (gtk_tree_model_get_iter_first(Valores_Destinatarios, &iterador))
	{
		do
		{
			gtk_tree_model_get(Valores_Destinatarios, &iterador, 0, &valor_linha, -1);
			g_ptr_array_add(array_de_empresas, g_strdup(valor_linha));
			g_free(valor_linha);
		}
		while (gtk_tree_model_iter_next(Valores_Destinatarios, &iterador));

		// Inserir cada destinatário
		for (guint i = 0; i < array_de_empresas->len; i++)
		{
			gchar *destinatario = g_ptr_array_index(array_de_empresas, i);
			char destinatario_escapado[512];
			mysql_real_escape_string(conexao, destinatario_escapado, destinatario, strlen(destinatario));

			char query[1200];
			snprintf(query, sizeof(query),
			         "INSERT INTO mensagens (titulo_mensagem, corpo_mensagem, empresa_destino, usuario_criador, data_de_criacao) "
			         "VALUES ('%s', '%s', '%s', '%s', '%s')",
			         titulo_escapado, texto_escapado, destinatario_escapado, Nome, data_hora);

			if (mysql_query(conexao, query))
			{
				fprintf(stderr, "Erro ao inserir dados: %s\n", mysql_error(conexao));
			}
		}
		mensagem("Sucesso","Mensagem Enviada com Sucesso",1,"");
		GtkWidget *mensagem_titulo_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_da_mensagem"));
		gtk_entry_set_text(GTK_ENTRY(mensagem_titulo_apagar), "");
		gtk_text_buffer_set_text(buffer, "", -1);
		gtk_list_store_clear(lista_de_selecionados);
	}
	else
	{
		mensagem("Erro", "Insira no Mínimo 1 Destinatário", 0, "Erro");
	}

	// Limpeza
	g_ptr_array_free(array_de_empresas, TRUE);
	g_free(texto);
	mysql_close(conexao);
}

//------------------------------------------------------------------------------------------Janela de Criacao abertura de Chamado----------------------------------------------------------------------------------------------

void on_abrir_chamado_janela(GtkButton *button, gpointer user_data)

{

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Abertura_de_chamado"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);


}

void on_Cancelar_chamado(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder,"Abertura_de_chamado"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
	GtkWidget *mensagem_titulo_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_do_Chamado"));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_do_Chamado"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	gtk_entry_set_text(GTK_ENTRY(mensagem_titulo_apagar), "");
	gtk_text_buffer_set_text(buffer, "", -1);

}

void on_Enviar_Chamado(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);

	struct tm *data_hora_local = localtime(&tempo_atual);
	snprintf(data_hora, sizeof(data_hora), "%04d-%02d-%02d %02d:%02d:%02d",
	         data_hora_local->tm_year + 1900, data_hora_local->tm_mon + 1,
	         data_hora_local->tm_mday, data_hora_local->tm_hour,
	         data_hora_local->tm_min, data_hora_local->tm_sec);

	gchar *Titulo_Chamado = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_do_Chamado")));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_do_Chamado"));


	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	GtkTextIter start, end;
	gchar *texto;

	// Obter o texto entre os iteradores
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	texto = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	// Verificar se o título ou o texto estão vazios
	if (g_strcmp0(Titulo_Chamado, "") == 0)
	{
		mensagem("Erro", "Insira um Título para o Chamado", 0, "Erro");
		g_free(texto);
		return;
	}
	if (g_strcmp0(texto, "") == 0)
	{
		mensagem("Erro", "Insira o texto do Chamado", 0, "Erro");
		g_free(texto);
		return;
	}

	// Preparar buffers de escape
	char titulo_escapado[512];
	char texto_escapado[512];
	mysql_real_escape_string(conexao, titulo_escapado, Titulo_Chamado, strlen(Titulo_Chamado));
	mysql_real_escape_string(conexao, texto_escapado, texto, strlen(texto));

	char query[1200];
	snprintf(query, sizeof(query),
	         "INSERT INTO chamados (titulo_chamado, corpo_chamado, nome_empresa,data_criacao)"
	         "VALUES ('%s', '%s', '%s', '%s')",
	         titulo_escapado, texto_escapado, Nome, data_hora);

	if (mysql_query(conexao, query))
	{
		fprintf(stderr, "Erro ao inserir dados: %s\n", mysql_error(conexao));
	}

	mensagem("Sucesso", "Chamado enviado com sucesso",1,"");
	GtkWidget *mensagem_titulo_apagar = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_do_Chamado"));
	gtk_entry_set_text(GTK_ENTRY(mensagem_titulo_apagar), "");
	gtk_text_buffer_set_text(buffer, "", -1);


// Limpeza
	g_free(texto);

    mysql_close(conexao);

}
//---------------------------------------------------------------------------------------------Janela de Historico e edicao de mensagens ----------------------------------------------------------------------------------------
void on_Abrir_historico(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkListStore *lista_de_selecionados = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_mensagem, corpo_mensagem, data_de_criacao, empresa_destino FROM mensagens ORDER BY id_mensagem DESC");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(lista_de_selecionados, &iter);
		gtk_list_store_set(lista_de_selecionados, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}
	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"HistoricoMensagens"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
	gtk_list_store_clear(lista_de_selecionados);


}
void on_Cancelar_Envio_das_edicoes(GtkButton *button, gpointer user_data)
{
	GtkListStore *lista_de_selecionados = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"HistoricoMensagens"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
	gtk_list_store_clear(lista_de_selecionados);

}


void on_Editar_Mensagem_on(GtkTreeView *Editar_Mensagem_on, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;
	gchar *nome_da_empresa = NULL;
	gchar *data_criacao = NULL;
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_da_Mensagems"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	GtkEntry *entry = GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_mensagem_edicao"));

	if (!list8)
	{
		g_warning("Liststore não encontrado!");
		return;
	}

	GtkTreeModel *model = gtk_tree_view_get_model(Editar_Mensagem_on);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, 4, &nome_da_empresa, 2, &data_criacao, -1);
		if (!nome_da_empresa || !data_criacao)
		{
			g_warning("nome_da_empresa ou data_criacao estão nulos");
			return;
		}
	}

	char Razao_empresas[250];
	snprintf(Razao_empresas, sizeof(Razao_empresas),
	         "SELECT id_mensagem, titulo_mensagem, corpo_mensagem FROM mensagens WHERE empresa_destino = '%s' AND data_de_criacao = '%s'",
	         nome_da_empresa, data_criacao);

	if (mysql_query(conexao, Razao_empresas) != 0)
	{
		g_print("Erro ao executar query: %s\n", mysql_error(conexao));
		return;
	}

	Dados_Mensagem = mysql_store_result(conexao);
	if (!Dados_Mensagem)
	{
		g_print("Erro ao armazenar resultado: %s\n", mysql_error(conexao));
		return;
	}

	while ((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		snprintf(id, sizeof(id), "%s", row[0]);
		gtk_entry_set_text(entry, row[1]);
		gtk_text_buffer_set_text(buffer, row[2], -1);
	}
	mysql_free_result(Dados_Mensagem);

	mysql_close(conexao);
	g_free(nome_da_empresa);
	g_free(data_criacao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_mensagens"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);


}

void on_Enviar_Edicao(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar Alteracao"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
}

void on_cancela_Envio(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_mensagem, corpo_mensagem, data_de_criacao, empresa_destino FROM mensagens ORDER BY id_mensagem DESC");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	mysql_free_result(Dados_Mensagem);

	mysql_close(conexao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar Alteracao"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

void on_Cancelar_Edicao(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_mensagem, corpo_mensagem, data_de_criacao, empresa_destino FROM mensagens ORDER BY id_mensagem DESC ");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_mensagens"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

void on_Confirmar_envio_mensagem(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);

	struct tm *data_hora_local = localtime(&tempo_atual);
	snprintf(data_hora, sizeof(data_hora), "%04d-%02d-%02d %02d:%02d:%02d",
	         data_hora_local->tm_year + 1900, data_hora_local->tm_mon + 1,
	         data_hora_local->tm_mday, data_hora_local->tm_hour,
	         data_hora_local->tm_min, data_hora_local->tm_sec);



	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_da_Mensagems"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	gchar *titulo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_mensagem_edicao")));
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gchar *texto = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	if(g_strcmp0(texto, "") != 0)
	{
		if(g_strcmp0(titulo, "") != 0)
		{
			char Alteracao_dados[1024];
			snprintf(Alteracao_dados,sizeof(Alteracao_dados),"UPDATE mensagens SET titulo_mensagem = '%s', corpo_mensagem = '%s',data_de_modificacao = '%s',usuario_modificador = '%s' WHERE id_mensagem = %s",titulo,texto,data_hora,Nome,id);
			if(mysql_query(conexao,Alteracao_dados))
			{
				g_print("Falha");

			}
			mensagem("Sucesso","Mensagem Alterada com Sucesso",1,"");
			GtkWidget *Titulo_do_Chamado = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_do_Chamado"));
			gtk_entry_set_text(GTK_ENTRY(Titulo_do_Chamado), "");
			gtk_text_buffer_set_text(buffer, "", -1);


			GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_mensagens"));
			GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
			gtk_widget_hide(dialogo_widget);
		}
		else
		{
			mensagem("Erro", "O corpo da mensagem não pode estar vazio", 0, "Erro");

		}
	}
	else
	{
		mensagem("Erro", "O Titulo não pode estar vazio", 0, "Erro");
	}
    gtk_list_store_clear(list8);
	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_mensagem, corpo_mensagem, data_de_criacao, empresa_destino FROM mensagens ORDER BY id_mensagem DESC ");

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	g_free(texto);

	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);



	GtkMessageDialog *mensagem_dialogo2 = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar Alteracao"));
	GtkWidget *dialogo_widget2 = GTK_WIDGET(mensagem_dialogo2);
	gtk_widget_hide(dialogo_widget2);


}


//---------------------------------------------------------------------------------------------Janela de Historico e edicao de Chamados ----------------------------------------------------------------------------------------
void on_Abrir_Historico_de_Chamados(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkListStore *lista_de_selecionados = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_chamado, corpo_chamado, data_criacao, nome_empresa FROM chamados where nome_empresa = '%s' ORDER BY id_chamado DESC ", Nome);

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(lista_de_selecionados, &iter);
		gtk_list_store_set(lista_de_selecionados, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}
	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"HistoricodeChamados"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
	gtk_list_store_clear(lista_de_selecionados);


}
void on_Cancelar_Envio_dos_chamados(GtkButton *button, gpointer user_data)
{
	GtkListStore *lista_de_selecionados = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data,"HistoricodeChamados"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
	gtk_list_store_clear(lista_de_selecionados);

}


void on_Editar_Chamado_on(GtkTreeView *Editar_Mensagem_on, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;
	gchar *nome_da_empresa = NULL;
	gchar *data_criacao = NULL;
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_do_Chamad"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	GtkEntry *entry = GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_Chamado_edicao"));

	if (!list8)
	{
		g_warning("Liststore não encontrado!");
		return;
	}

	GtkTreeModel *model = gtk_tree_view_get_model(Editar_Mensagem_on);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, 4, &nome_da_empresa, 2, &data_criacao, -1);
		if (!data_criacao)
		{
			g_warning("nome_da_empresa ou data_criacao estão nulos");
			return;
		}
	}

	char Razao_empresas[250];
	snprintf(Razao_empresas, sizeof(Razao_empresas),
	         "SELECT id_chamado, titulo_chamado, corpo_chamado FROM chamados WHERE data_criacao = '%s'",
	         data_criacao);

	if (mysql_query(conexao, Razao_empresas) != 0)
	{
		g_print("Erro ao executar query: %s\n", mysql_error(conexao));
		return;
	}

	Dados_Mensagem = mysql_store_result(conexao);
	if (!Dados_Mensagem)
	{
		g_print("Erro ao armazenar resultado: %s\n", mysql_error(conexao));
		return;
	}

	while ((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		snprintf(id, sizeof(id), "%s", row[0]);
		gtk_entry_set_text(entry, row[1]);
		gtk_text_buffer_set_text(buffer, row[2], -1);
	}
	mysql_free_result(Dados_Mensagem);

	mysql_close(conexao);
	g_free(nome_da_empresa);
	g_free(data_criacao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_chamados"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);


}

void on_Enviar_Chamado_editado(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar_Chamado"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_show_all(dialogo_widget);
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(dialogo_widget);
}

void on_cancela_Chamado(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_chamado, corpo_chamado, data_criacao, nome_empresa FROM chamados where nome_empresa = '%s' ORDER BY id_chamado DESC ", Nome);

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	mysql_free_result(Dados_Mensagem);

	mysql_close(conexao);
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar_Chamado"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

void on_Cancelar_Envio_Chamado(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_chamado, corpo_chamado, data_criacao, nome_empresa FROM chamados where nome_empresa = '%s' ORDER BY id_chamado DESC ", Nome);

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_chamados"));
	GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
	gtk_widget_hide(dialogo_widget);
}

void on_Confirmar_envio_Chamado(GtkButton *button, gpointer user_data)
{
	GtkListStore *list8 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore8"));
	gtk_list_store_clear(list8);

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem;
	MYSQL_ROW row;

	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);

	struct tm *data_hora_local = localtime(&tempo_atual);
	snprintf(data_hora, sizeof(data_hora), "%04d-%02d-%02d %02d:%02d:%02d",
	         data_hora_local->tm_year + 1900, data_hora_local->tm_mon + 1,
	         data_hora_local->tm_mday, data_hora_local->tm_hour,
	         data_hora_local->tm_min, data_hora_local->tm_sec);



	GtkTextView *CorpoMensagem = GTK_TEXT_VIEW(gtk_builder_get_object(user_data, "Corpo_do_Chamad"));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(CorpoMensagem);
	gchar *titulo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Titulo_Chamado_edicao")));
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gchar *texto = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	if(g_strcmp0(texto, "") != 0)
	{
		if(g_strcmp0(titulo, "") != 0)
		{
			char Alteracao_dados[1024];
			snprintf(Alteracao_dados,sizeof(Alteracao_dados),"UPDATE chamados SET titulo_chamado = '%s', corpo_chamado = '%s', data_de_modificacao = '%s', usuario_modificador = '%s' WHERE id_chamado = %s",titulo,texto,data_hora,Nome,id);
			if(mysql_query(conexao,Alteracao_dados))
			{
				g_print("Falha");

			}
			mensagem("Sucesso","Chamado Alterado com Sucesso",1,"");
			GtkWidget *Titulo_do_Chamado = GTK_WIDGET(gtk_builder_get_object(GTK_BUILDER(user_data), "Titulo_Chamado_edicao"));
			gtk_entry_set_text(GTK_ENTRY(Titulo_do_Chamado), "");
			gtk_text_buffer_set_text(buffer, "", -1);


			GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Edicao_de_chamados"));
			GtkWidget *dialogo_widget = GTK_WIDGET(mensagem_dialogo);
			gtk_widget_hide(dialogo_widget);
		}
		else
		{
			mensagem("Erro", "O corpo do chamado não pode estar vazio", 0, "Erro");

		}
	}
	else
	{
		mensagem("Erro", "O Titulo não pode estar vazio", 0, "Erro");
	}

	GtkTreeIter iter;

	char Razao_empresas[250];

	snprintf(Razao_empresas, sizeof(Razao_empresas),"SELECT titulo_chamado, corpo_chamado, data_criacao, nome_empresa FROM chamados where nome_empresa = '%s' ORDER BY id_chamado DESC ", Nome);

	if(mysql_query(conexao,Razao_empresas))
	{
		g_print("Deu ruim");
	}
	Dados_Mensagem = mysql_store_result(conexao);
	if(Dados_Mensagem == NULL)
	{
		g_print("Deu ruim mais ainda");
	}
	while((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
	{
		char Clique[50];
		snprintf(Clique, sizeof(Clique),"Para Editar Clique Aqui");
		gtk_list_store_append(list8, &iter);
		gtk_list_store_set(list8, &iter, 0, row[0], 1, row[1], 2, row[2], 4, row[3],3, Clique,-1);
	}

	g_free(texto);

	mysql_free_result(Dados_Mensagem);
	mysql_close(conexao);



	GtkMessageDialog *mensagem_dialogo2 = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Confirmar_Chamado"));
	GtkWidget *dialogo_widget2 = GTK_WIDGET(mensagem_dialogo2);
	gtk_widget_hide(dialogo_widget2);


}
//----------------------------------------------------------------------------------Relatorio Detalhado-------------------------------------------------------------------------------------------------------------------------
void on_Relatorio_Detalhado(GtkWidget *Relatorio, GdkEventButton *event, gpointer user_data)
{
	GtkListStore *store9 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore9"));

	MYSQL *conexao = obterConexao();
	MYSQL_RES *Dados_Mensagem = NULL, *Dados_Mensagem2 = NULL,*Dados_Confirma = NULL;
	MYSQL_ROW row;

	GtkTreeIter iter;

	char Razao_empresas[250];
	snprintf(Razao_empresas, sizeof(Razao_empresas), "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores WHERE empresa = '%s' ORDER BY id_relatorio DESC", Nome);

	if (mysql_query(conexao, Razao_empresas) == 0)
	{
		Dados_Mensagem = mysql_store_result(conexao);
	}
	else
	{
		fprintf(stderr, "Erro na consulta: %s\n", mysql_error(conexao));
	}

	char Adm[500];
	snprintf(Adm, sizeof(Adm), "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores ORDER BY id_relatorio DESC");
	if (mysql_query(conexao, Adm) == 0)
	{
		Dados_Mensagem2 = mysql_store_result(conexao);
	}
	else
	{
		fprintf(stderr, "Erro na consulta: %s\n", mysql_error(conexao));
	}

	if (Dados_Mensagem != NULL && mysql_num_rows(Dados_Mensagem) > 0)
	{
		while ((row = mysql_fetch_row(Dados_Mensagem)) != NULL)
		{

			char valores[500];

			formatar_valor_em_reais(row[2],valores,sizeof(valores));
			gtk_list_store_append(store9, &iter);
			gtk_list_store_set(store9, &iter, 0, row[0], 1, row[4], 2, row[3], 3, valores, 4, row[1], -1);
		}
		if(Dados_Confirma)
			mysql_free_result(Dados_Confirma);
		if (Dados_Mensagem)
		{
			mysql_free_result(Dados_Mensagem);
		}
		if (Dados_Mensagem2)
		{
			mysql_free_result(Dados_Mensagem2);
		}
		mysql_close(conexao);

		GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder, "Relatorios_Detalhados_de_Valores_empresa"));
		gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
		gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
		gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
		gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
		gtk_list_store_clear(store9);
	}

	else if (Adm != NULL && mysql_num_rows(Dados_Mensagem2) > 0)
	{
		char AdmValida[500];
		snprintf(AdmValida, sizeof(AdmValida), "SELECT nome FROM usuarios_admins WHERE nome_usuario = '%s'",Nome);

		if(mysql_query(conexao,AdmValida)==0)
		{
			Dados_Confirma = mysql_store_result(conexao);
			if(Dados_Confirma != NULL && mysql_num_rows(Dados_Confirma) > 0)
			{


				while ((row = mysql_fetch_row(Dados_Mensagem2)) != NULL)
				{

					char valores[500];
					formatar_valor_em_reais(row[2],valores,sizeof(valores));
					gtk_list_store_append(store9, &iter);
					gtk_list_store_set(store9, &iter, 0, row[0], 1, row[4], 2, row[3], 3,valores, 4, row[1], -1);
				}
				if(Dados_Confirma)
					mysql_free_result(Dados_Confirma);
				if (Dados_Mensagem)
				{
					mysql_free_result(Dados_Mensagem);
				}
				if (Dados_Mensagem2)
				{
					mysql_free_result(Dados_Mensagem2);
				}
				mysql_close(conexao);

				GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(builder, "Relatorios_Detalhados_de_Valores_ADM"));
				gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
				gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
				gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
				gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
				gtk_list_store_clear(store9);
			}
		}

	}

}

void on_Sair_Relatorio_Detalhado(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Relatorios_Detalhados_de_Valores_ADM"));
	gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
	GtkListStore *store9 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore9"));
	gtk_list_store_clear(store9);
}
void on_Sair_Relatorio_Detalhado1(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "Relatorios_Detalhados_de_Valores_empresa"));
	gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
	GtkListStore *store9 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore9"));
	gtk_list_store_clear(store9);
}
//----------------------------------------------------------------------Pesquisar Relatorios tabela -------------------------------------------------------------------------------------------------------------------------------
void on_pesquisar_relatorios(GtkEntry *entry,gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	if (!conexao)
	{
		mensagem("Erro", "Não foi possível estabelecer conexão com o banco de dados", 0, "erro");
		return;
	}

			GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
			GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
			GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);

	gchar *data = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_Data")));
	gchar *nome_empresa = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_nome_empresa")));
	gchar *nome_arquivo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_nome_arquivo")));

	GtkListStore *list4 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore4"));

	if (!list4)
	{
		mensagem("Erro", "Erro ao acessar ListStore", 0, "erro");
		return;
	}

	// Limpa a ListStore antes de preencher
	gtk_list_store_clear(list4);

	char query[700];
	snprintf(query, sizeof(query), "SELECT nome_arquivo, data, empresa FROM relatorio_valores WHERE 1=1");

	if (g_strcmp0(nome_arquivo, "") != 0)
	{
		strncat(query, " AND nome_arquivo LIKE '%", sizeof(query) - strlen(query) - 1);
		strncat(query, nome_arquivo, sizeof(query) - strlen(query) - 1);
		strncat(query, "%'", sizeof(query) - strlen(query) - 1);
	}

	if (g_strcmp0(nome_empresa, "") != 0)
	{
		strncat(query, " AND empresa LIKE '%", sizeof(query) - strlen(query) - 1);
		strncat(query, nome_empresa, sizeof(query) - strlen(query) - 1);
		strncat(query, "%'", sizeof(query) - strlen(query) - 1);
	}

	if (g_strcmp0(data, "") != 0)
	{
		strncat(query, " AND data LIKE '%", sizeof(query) - strlen(query) - 1);
		strncat(query, data, sizeof(query) - strlen(query) - 1);
		strncat(query, "%'", sizeof(query) - strlen(query) - 1);
	}

	strncat(query, " ORDER BY id_relatorio DESC", sizeof(query) - strlen(query) - 1);

	if (mysql_query(conexao, query) == 0)
	{
		MYSQL_RES *dados_mensagem = mysql_store_result(conexao);
		if (dados_mensagem && mysql_num_rows(dados_mensagem) > 0)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(dados_mensagem)) != NULL)
			{
				GtkTreeIter iter;
				gtk_list_store_append(list4, &iter);
				gtk_list_store_set(list4, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, row[2] ? row[2] : "", 3, pixbuf_xls, 4, pixbuf_csv, 5, pixbuf_txt, -1);
			}
		}
		mysql_free_result(dados_mensagem);  // Libera os resultados da consulta
	}
	else
	{
		mensagem("Erro", "Falha ao executar a consulta: ", 0, "erro");
	}
	mysql_close(conexao);
}
void on_pesquisar_relatorio_empresas(GtkEntry *entry,gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	if (!conexao)
	{
		mensagem("Erro", "Não foi possível estabelecer conexão com o banco de dados", 0, "erro");
		return;
	}
			GdkPixbuf *pixbuf_xls = gdk_pixbuf_new_from_file("./glade/css/imgs/xls.png", NULL);
			GdkPixbuf *pixbuf_txt = gdk_pixbuf_new_from_file("./glade/css/imgs/txt.png", NULL);
			GdkPixbuf *pixbuf_csv = gdk_pixbuf_new_from_file("./glade/css/imgs/csv.png", NULL);

	gchar *data = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_Data1")));
	gchar *nome_arquivo = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_nome_arquivo1")));

	GtkListStore *list4 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore1"));

	if (!list4)
	{
		mensagem("Erro", "Erro ao acessar ListStore", 0, "erro");
		return;
	}

	// Limpa a ListStore antes de preencher
	gtk_list_store_clear(list4);

	char query[700];
	snprintf(query, sizeof(query), "SELECT nome_arquivo, data FROM relatorio_valores WHERE 1=1 AND empresa= '%s'",Nome);

	if (g_strcmp0(nome_arquivo, "") != 0)
	{
		strncat(query, " AND nome_arquivo LIKE '%", sizeof(query) - strlen(query) - 1);
		strncat(query, nome_arquivo, sizeof(query) - strlen(query) - 1);
		strncat(query, "%'", sizeof(query) - strlen(query) - 1);
	}


	if (g_strcmp0(data, "") != 0)
	{
		strncat(query, " AND data LIKE '%", sizeof(query) - strlen(query) - 1);
		strncat(query, data, sizeof(query) - strlen(query) - 1);
		strncat(query, "%'", sizeof(query) - strlen(query) - 1);
	}

	strncat(query, " ORDER BY id_relatorio DESC", sizeof(query) - strlen(query) - 1);

	if (mysql_query(conexao, query) == 0)
	{
		MYSQL_RES *dados_mensagem = mysql_store_result(conexao);
		if (dados_mensagem && mysql_num_rows(dados_mensagem) > 0)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(dados_mensagem)) != NULL)
			{
				GtkTreeIter iter;
				gtk_list_store_append(list4, &iter);
				gtk_list_store_set(list4, &iter, 0, row[0] ? row[0] : "", 1, row[1] ? row[1] : "", 2, pixbuf_xls, 3,pixbuf_csv, 4,pixbuf_txt, -1);
			}
		}
		mysql_free_result(dados_mensagem);  // Libera os resultados da consulta
	}
	else
	{
		mensagem("Erro", "Falha ao executar a consulta: ", 0, "erro");
	}
	mysql_close(conexao);
}
//--------------------------------------------------------------------------------------Pesquisas de Relátorio Geral--------------------------------------------------------------------------------------------------------------

void on_Fazer_Pesquisa_Entre_datas1(GtkButton *button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");
		return;
	}

	MYSQL_RES *Valor_Final = NULL;
	MYSQL_ROW row;
	GtkListStore *list9 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore9"));
	GtkTreeIter iter;

	gchar *data_inicial = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_Data_Inicial1")));
	gchar *data_final = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_Data_Final1")));

	char query[700];

	// Verifica se as datas são válidas e se ambas estão vazias
	if ((data_inicial == NULL || *data_inicial == '\0') && (data_final == NULL || *data_final == '\0'))
	{
		// Consulta padrão para carregar todos os dados da tabela
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE empresa = '%s' ORDER BY id_relatorio DESC", Nome);
		g_print("Nenhuma data inserida. Carregando todos os registros.\n");
	}
	// Caso as duas datas estejam preenchidas, realiza a busca entre elas
	else if (data_inicial != NULL && *data_inicial != '\0' && data_final != NULL && *data_final != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE empresa = '%s' AND data BETWEEN '%s 00:00:00' AND '%s 23:59:59' ORDER BY id_relatorio DESC",
		         Nome, data_inicial, data_final);
	}
	// Caso apenas data_inicial esteja preenchida
	else if (data_inicial != NULL && *data_inicial != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE empresa = '%s' AND data >= '%s 00:00:00' ORDER BY id_relatorio DESC",
		         Nome, data_inicial);
	}
	// Caso apenas data_final esteja preenchida
	else if (data_final != NULL && *data_final != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE empresa = '%s' AND data <= '%s 23:59:59' ORDER BY id_relatorio DESC",
		         Nome, data_final);
	}

	// Executa a consulta
	if (mysql_query(conexao, query) == 0)
	{
		Valor_Final = mysql_store_result(conexao);
		if (Valor_Final != NULL && mysql_num_rows(Valor_Final) > 0)
		{
			gtk_list_store_clear(list9);
			while ((row = mysql_fetch_row(Valor_Final)) != NULL)
			{
				char valores[500];
				if (row[2] != NULL)
				{
					formatar_valor_em_reais(row[2], valores, sizeof(valores));
				}
				else
				{
					strncpy(valores, "N/A", sizeof(valores));
				}
				gtk_list_store_append(list9, &iter);
				gtk_list_store_set(list9, &iter, 0, row[0], 1, row[4], 2, row[3], 3, valores, 4, row[1], -1);
			}
		}
		else
		{
			g_print("Nenhum resultado encontrado para o(s) critério(s) de data fornecido(s).\n");
			gtk_list_store_clear(list9);
			mensagem("Erro", "Nenhum resultado encontrado para o intervalo de datas fornecido.", 0, "Erro");
		}
	}
	else
	{
		g_print("Erro ao executar a consulta no banco de dados. Verifique as datas.\n");
		mensagem("Erro", "Erro ao executar a consulta no banco de dados", 0, "Erro");
	}

	if (Valor_Final != NULL)
	{
		mysql_free_result(Valor_Final);
	}
	mysql_close(conexao);
}


void on_Fazer_Pesquisa_Entre_datas(GtkButton *button, gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");
		return;
	}

	MYSQL_RES *Valor_Final = NULL;
	MYSQL_ROW row;
	GtkListStore *list9 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore9"));
	GtkTreeIter iter;

	gchar *data_inicial = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_Data_Inicial")));
	gchar *data_final = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_por_Data_Final")));

	char query[700];

	// Verifica se as datas são válidas
	if ((data_inicial == NULL || *data_inicial == '\0') && (data_final == NULL || *data_final == '\0'))
	{
		// Consulta padrão sem filtro de data para carregar todos os registros
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores ORDER BY id_relatorio DESC");
		g_print("Nenhuma data inserida. Carregando todos os registros.\n");
	}
	// Caso as duas datas estejam preenchidas, realiza a busca entre elas
	else if (data_inicial != NULL && *data_inicial != '\0' && data_final != NULL && *data_final != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE data BETWEEN '%s 00:00:00' AND '%s 23:59:59' ORDER BY id_relatorio DESC",
		         data_inicial, data_final);
	}
	// Caso apenas data_inicial esteja preenchida
	else if (data_inicial != NULL && *data_inicial != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE data >= '%s 00:00:00' ORDER BY id_relatorio DESC",
		         data_inicial);
	}
	// Caso apenas data_final esteja preenchida
	else if (data_final != NULL && *data_final != '\0')
	{
		snprintf(query, sizeof(query),
		         "SELECT nome_arquivo, data, valor_custo, quantidade_residuos, empresa FROM relatorio_valores "
		         "WHERE data <= '%s 23:59:59' ORDER BY id_relatorio DESC",
		         data_final);
	}

	// Executa a consulta
	if (mysql_query(conexao, query) == 0)
	{
		Valor_Final = mysql_store_result(conexao);
		if (Valor_Final != NULL && mysql_num_rows(Valor_Final) > 0)
		{
			// Limpa o liststore antes de adicionar novos dados
			gtk_list_store_clear(list9);

			while ((row = mysql_fetch_row(Valor_Final)) != NULL)
			{
				char valores[500];
				if (row[2] != NULL)
				{
					formatar_valor_em_reais(row[2], valores, sizeof(valores));
				}
				else
				{
					strncpy(valores, "N/A", sizeof(valores));
				}
				gtk_list_store_append(list9, &iter);
				gtk_list_store_set(list9, &iter, 0, row[0], 1, row[4], 2, row[3], 3, valores, 4, row[1], -1);
			}
		}
		else
		{
			g_print("Nenhum resultado encontrado para o(s) critério(s) de data fornecido(s).\n");
			mensagem("Erro", "Nenhum resultado encontrado para o intervalo de datas fornecido.", 0, "Erro");
		}
	}
	else
	{
		g_print("Erro ao executar a consulta no banco de dados. Verifique as datas.\n");
		mensagem("Erro", "Erro ao executar a consulta no banco de dados", 0, "Erro");
	}

	if (Valor_Final != NULL)
	{
		mysql_free_result(Valor_Final);
	}
	mysql_close(conexao);
}


void on_Gerar_CSV(GtkWidget *Relatorio, GdkEventButton *event, gpointer user_data)
{
	geral = 1;
	on_open_file_chooser(user_data);

}

//------------------------------------------------------------------------------------------------Chat de Usuarios-----------------------------------------------------------------------------------------------------------------


void CarregarChat()
{
	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);
	struct tm *data_hora_local = localtime(&tempo_atual);
	sprintf(data_hora, "%04d-%02d-%02d %02d:%02d:%02d",
	        data_hora_local->tm_year + 1900,
	        data_hora_local->tm_mon + 1,
	        data_hora_local->tm_mday,
	        data_hora_local->tm_hour,
	        data_hora_local->tm_min,
	        data_hora_local->tm_sec);

	char hora[20];
	sprintf(hora, "%02d:%02d", data_hora_local->tm_hour, data_hora_local->tm_min);

	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");

	}

	MYSQL_RES *resultado = NULL;
	MYSQL_ROW row;
	gchar *Remetente = gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(builder, "Nome_Empresa")));

	char Pesquisa_Usuario_Atual[1024];
	char formatacao[250];
	GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder, "Janela de Scrool"));
	GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(scrolled_window);
	GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
	GtkTreeIter iter;

	char setarLida[250];
	snprintf(setarLida,sizeof(setarLida),"UPDATE bate_papo SET lida = TRUE WHERE nome_do_remetente = '%s' AND nome_do_destinatario = '%s' ",Remetente,Nome);
	if(mysql_query(conexao,setarLida))
	{
		g_print("Erro");
	}



	snprintf(Pesquisa_Usuario_Atual, sizeof(Pesquisa_Usuario_Atual),
	         "SELECT nome_do_remetente,nome_do_destinatario,conteudo,mensagem_referente, DATE(hora_envio) AS diferenca_dias, DATE_FORMAT(hora_envio, '%%H:%%i') AS hora_minutos,id FROM bate_papo WHERE nome_do_remetente = '%s' AND nome_do_destinatario = '%s' AND id > %s  OR  nome_do_remetente = '%s' AND nome_do_destinatario = '%s' AND id > %s ",
	         Nome, Remetente,hora_mensagem, Remetente, Nome,hora_mensagem);

	if (mysql_query(conexao, Pesquisa_Usuario_Atual) == 0)
	{
		resultado = mysql_store_result(conexao);

		if (resultado != NULL && mysql_num_rows(resultado) > 0)
		{

			while ((row = mysql_fetch_row(resultado)))
			{
				char Data[250];
				char Mesagens[500];

				snprintf(Data, sizeof(Data), "%s", row[4]);

				if (strcmp(Data, Datatabela) == 0 )
				{
					// Lógica opcional
				}
				else
				{
					snprintf(Datatabela, sizeof(Datatabela), "%s", Data);

					snprintf(formatacao, sizeof(formatacao), "%s                                                                        ", Datatabela);

					gtk_list_store_append(list10, &iter);
					gtk_list_store_set(list10, &iter, 0, "", 1, formatacao, -1);
				}
				char UsuarioRemet[250];
				snprintf(UsuarioRemet,sizeof(UsuarioRemet),"%s",row[1]);
				if (strcmp(UsuarioRemet, Nome) == 0 )
				{
					if(row[3] == NULL)
					{
						char MensagemFormatado[1000];
						snprintf(MensagemFormatado,sizeof(MensagemFormatado),"%s \n %s",row[2],row[5]);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,MensagemFormatado, 1, "", -1);
					}
					else
					{
						char MensagemFormatado[1000];
						char segundaformat[500];
						snprintf(MensagemFormatado,sizeof(MensagemFormatado),"Respondendo há:%s ",row[3]);
						snprintf(segundaformat,sizeof(segundaformat),"%s \n %s",row[2],row[5]);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,MensagemFormatado, 1,"", -1);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,segundaformat, 1,"", -1);
					}
				}
				else
				{
					if(row[3] == NULL)
					{
						char MensagemFormatado[1000];
						snprintf(MensagemFormatado,sizeof(MensagemFormatado),"%s \n %s",row[2],row[5]);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,"", 1, MensagemFormatado, -1);
					}
					else
					{
						char MensagemFormatado[1000];
						char segundaformat[500];
						snprintf(MensagemFormatado,sizeof(MensagemFormatado),"Respondendo há:%s ",row[3]);
						snprintf(segundaformat,sizeof(segundaformat),"%s \n %s",row[2],row[5]);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,"", 1, MensagemFormatado, -1);
						gtk_list_store_append(list10, &iter);
						gtk_list_store_set(list10, &iter, 0,"", 1, segundaformat, -1);
					}
				}

				snprintf(hora_mensagem,sizeof(hora_mensagem),"%s",row[6]);
				gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment));

			}
			mysql_free_result(resultado);
		}

	}
	mysql_close(conexao);

}

gboolean Carregar_Chat_Segundos(GtkLabel *label)
{

	CarregarChat();
	return TRUE;
}

void on_Tabela_Chamados_responder(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{

	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");
		return;
	}

	MYSQL_RES *Valor_Final = NULL;
	MYSQL_ROW row;

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *prev_path;
	gchar *valor_data;
	gchar *empresa_referente;

	model = gtk_tree_view_get_model(treeview);

	// Obtém o iterador da linha clicada
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		// Obtém o valor da linha clicada (opcional)
		gtk_tree_model_get(model, &iter, 0, &valor_data, -1);
		valor_data = str_replace(valor_data,"Data de Criação: ","");
		valor_data = str_replace(valor_data," Para Responder Clique Aqui!!","");



		// Percorre as três linhas anteriores
		for (int i = 1; i <= 1; i++)
		{
			// Clona o caminho da linha selecionada e recua uma linha
			prev_path = gtk_tree_path_copy(path);
			if (gtk_tree_path_prev(prev_path))    // Move o caminho uma linha para trás
			{
				// Obtém o iterador da linha anterior
				if (gtk_tree_model_get_iter(model, &iter, prev_path))
				{
					gtk_tree_model_get(model, &iter, 0, &empresa_referente, -1);
					empresa_referente = str_replace(empresa_referente,"Empresa Referente: ","");
					empresa_referente = str_replace(empresa_referente,"Criado por: ","");

					char AdmValida[500];
					snprintf(AdmValida, sizeof(AdmValida), "SELECT titulo_chamado, nome_empresa FROM chamados WHERE nome_empresa = '%s' AND data_criacao = '%s'",empresa_referente,valor_data);
					if(mysql_query(conexao,AdmValida)==0)
					{
						Valor_Final= mysql_store_result(conexao);
						if(Valor_Final != NULL && mysql_num_rows(Valor_Final)>0)
						{
							while(row = mysql_fetch_row(Valor_Final))
							{
								char Lido[500];
								snprintf(Lido, sizeof(Lido), "UPDATE chamados SET lido = TRUE WHERE nome_empresa = '%s' AND data_criacao = '%s'",empresa_referente,valor_data);
								if(mysql_query(conexao,Lido)) {}

								GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa")));
								gtk_label_set_text(Mensagem,row[1]);
								GtkLabel *MensagemResposta = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
								char ValorMensagem[250];
								snprintf(ValorMensagem,sizeof(ValorMensagem),"CHAMADO %s",row[0]);
								gtk_label_set_text(MensagemResposta,ValorMensagem);

								CarregarChat();
								GtkWidget *label = gtk_label_new(NULL);

								carregar_chat = g_timeout_add_seconds(1, (GSourceFunc)Carregar_Chat_Segundos,label);

								GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "BatePapo"));
								gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
								gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
								gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
								gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
								g_source_remove(carregar_chat);
								carregar_chat = 0;
								GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
								gtk_list_store_clear(list10);
								snprintf(hora_mensagem,sizeof(hora_mensagem),"0");


							}


						}
						else
						{
							mysql_free_result(Valor_Final);
							char validar[500];
							snprintf(validar, sizeof(validar), "SELECT titulo_mensagem, usuario_criador FROM mensagens WHERE usuario_criador = '%s' AND data_de_criacao = '%s'",empresa_referente,valor_data);
							if(mysql_query(conexao,validar)==0)
							{
								Valor_Final= mysql_store_result(conexao);
								if(Valor_Final != NULL && mysql_num_rows(Valor_Final)>0)
								{


									while(row = mysql_fetch_row(Valor_Final))
									{
										char Lido[500];
										snprintf(Lido, sizeof(Lido), "UPDATE Mensagens SET lido = TRUE WHERE usuario_criador = '%s' AND data_de_criacao = '%s'",empresa_referente,valor_data);
										if(mysql_query(conexao,Lido)) {}

										GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa")));
										gtk_label_set_text(Mensagem,row[1]);
										GtkLabel *MensagemResposta = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
										char ValorMensagem[250];
										snprintf(ValorMensagem,sizeof(ValorMensagem),"Mensagem %s",row[0]);
										gtk_label_set_text(MensagemResposta,ValorMensagem);

										CarregarChat();
										GtkWidget *label = gtk_label_new(NULL);

										carregar_chat = g_timeout_add_seconds(1, (GSourceFunc)Carregar_Chat_Segundos,label);

										GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "BatePapo"));
										gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
										gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
										gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
										gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
										g_source_remove(carregar_chat);
										carregar_chat = 0;
										GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
										gtk_list_store_clear(list10);
										snprintf(hora_mensagem,sizeof(hora_mensagem),"0");


									}

								}

							}
						}




						g_free(empresa_referente);
					}
				}
				else
				{
					g_print("Sem mais linhas antes da linha selecionada.\n");
					gtk_tree_path_free(prev_path);
					break;
				}
				gtk_tree_path_free(prev_path); // Libera a memória do caminho clonado
			}
		}
		g_free(valor_data);
	}
	mysql_close(conexao);
}

void CarregarLidas(int Somente_nao_lidas)
{

	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");

	}
	GtkListStore *list11 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore11"));
	GtkTreeIter iter;

	MYSQL_RES *resultado = NULL, *outro_resultado = NULL;
	MYSQL_ROW row, coluna;
	char Pesquisar_Tabelas[250];
	snprintf(Pesquisar_Tabelas,sizeof(Pesquisar_Tabelas),"SELECT * FROM usuarios_empresas WHERE razao_social = '%s'",Nome);
	if(Somente_nao_lidas == 0)
	{

		if(mysql_query(conexao,Pesquisar_Tabelas)==0)
		{
			resultado = mysql_store_result(conexao);
			if(resultado != NULL && mysql_num_rows(resultado)>0)
			{
				mysql_free_result(resultado);
				char Pesquisar_ADMS[250];
				snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins");
				if(mysql_query(conexao,Pesquisar_ADMS)==0)
				{
					resultado = mysql_store_result(conexao);
					if(resultado != NULL && mysql_num_rows(resultado)>0)
					{
						while(row = mysql_fetch_row(resultado))
						{
							char Pendencia[250];
							snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome) ;
							if(mysql_query(conexao,Pendencia)==0)
							{
								outro_resultado = mysql_store_result(conexao);
								if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
								{
									while(coluna = mysql_fetch_row(outro_resultado))
									{
										char naovistas[250];
										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}

								}
								else
								{
									char naovistas[250];
									snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
									gtk_list_store_append(list11, &iter);
									gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
								}
								mysql_free_result(outro_resultado);
							}
						}
					}
					mysql_free_result(resultado);
				}

			}
			else
			{


				mysql_free_result(resultado);
				char Pesquisar_empresa[250];
				snprintf(Pesquisar_empresa,sizeof(Pesquisar_empresa),"SELECT razao_social FROM usuarios_empresas");
				if(mysql_query(conexao,Pesquisar_empresa)==0)
				{
					resultado = mysql_store_result(conexao);
					if(resultado != NULL && mysql_num_rows(resultado)>0)
					{
						while(row = mysql_fetch_row(resultado))
						{

							char Pendencia[250];
							snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
							if(mysql_query(conexao,Pendencia)==0)
							{
								outro_resultado= mysql_store_result(conexao);
								if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
								{
									while(coluna = mysql_fetch_row(outro_resultado))
									{

										char naovistas[250];

										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}

								}
								else
								{
									char naovistas[250];
									snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
									gtk_list_store_append(list11, &iter);
									gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
								}
								mysql_free_result(outro_resultado);

							}
						}
					}
				}
				mysql_free_result(resultado);
				char Pesquisar_ADMS[250];
				snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins");
				if(mysql_query(conexao,Pesquisar_ADMS)==0)
				{
					resultado = mysql_store_result(conexao);
					if(resultado != NULL && mysql_num_rows(resultado)>0)
					{
						while(row = mysql_fetch_row(resultado))
						{
							char Pendencia[250];
							snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
							if(mysql_query(conexao,Pendencia)==0)
							{
								outro_resultado=mysql_store_result(conexao);
								if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
								{
									while(coluna = mysql_fetch_row(outro_resultado))
									{
										char naovistas[250];
										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}

								}
								else
								{
									char naovistas[250];
									snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
									gtk_list_store_append(list11, &iter);
									gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
								}
								mysql_free_result(outro_resultado);
							}
						}
					}
					mysql_free_result(resultado);
				}
			}
		}
		else
		{
			g_print("Erro");
		}

	}
	gchar *Usuarios = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_Usuarios")));
	if(Somente_nao_lidas == 1)
	{
		gtk_list_store_clear(list11);
		if(g_strcmp0(Usuarios,"")!= 0)
		{
			if(mysql_query(conexao,Pesquisar_Tabelas)==0)
			{
				resultado = mysql_store_result(conexao);
				if(resultado != NULL && mysql_num_rows(resultado)>0)
				{
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins WHERE nome_usuario LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome) ;
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado = mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											char naovistas[250];
											snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
											gtk_list_store_append(list11, &iter);
											gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
										}

									}
									else
									{
										char naovistas[250];
										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}
									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}

				}
				else
				{


					mysql_free_result(resultado);
					char Pesquisar_empresa[250];
					snprintf(Pesquisar_empresa,sizeof(Pesquisar_empresa),"SELECT razao_social FROM usuarios_empresas WHERE razao_social LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_empresa)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{

								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado= mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										g_print("Chegou\n");
										while(coluna = mysql_fetch_row(outro_resultado))
										{

											char naovistas[250];

											snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
											gtk_list_store_append(list11, &iter);
											gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
										}

									}
									else
									{
										char naovistas[250];
										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}
									mysql_free_result(outro_resultado);
								}
							}
						}
					}
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins WHERE nome_usuario LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado=mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											char naovistas[250];
											snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
											gtk_list_store_append(list11, &iter);
											gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
										}

									}
									else
									{
										char naovistas[250];
										snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : 0");
										gtk_list_store_append(list11, &iter);
										gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
									}
									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}
				}
			}
		}
		else
		{
			int x=0;
			CarregarLidas(x);

		}


	}
	if(Somente_nao_lidas == 2 )
	{
		gtk_list_store_clear(list11);
		if(g_strcmp0(Usuarios,"")!= 0)
		{
			if(mysql_query(conexao,Pesquisar_Tabelas)==0)
			{
				resultado = mysql_store_result(conexao);
				if(resultado != NULL && mysql_num_rows(resultado)>0)
				{
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins WHERE nome_usuario LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome) ;
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado = mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}

									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}

				}
				else
				{
					mysql_free_result(resultado);
					char Pesquisar_empresa[250];
					snprintf(Pesquisar_empresa,sizeof(Pesquisar_empresa),"SELECT razao_social FROM usuarios_empresas WHERE razao_social LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_empresa)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{

								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado= mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{

											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}
									mysql_free_result(outro_resultado);

								}
							}
						}
					}
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins WHERE nome_usuario LIKE '%%%s%%' ",Usuarios);
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado=mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}

									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}
				}
			}
		}
		else
		{

			if(mysql_query(conexao,Pesquisar_Tabelas)==0)
			{
				resultado = mysql_store_result(conexao);
				if(resultado != NULL && mysql_num_rows(resultado)>0)
				{
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins");
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome) ;
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado = mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}

									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}

				}
				else
				{
					mysql_free_result(resultado);
					char Pesquisar_empresa[250];
					snprintf(Pesquisar_empresa,sizeof(Pesquisar_empresa),"SELECT razao_social FROM usuarios_empresas");
					if(mysql_query(conexao,Pesquisar_empresa)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{

								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado= mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{

											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}
									mysql_free_result(outro_resultado);

								}
							}
						}
					}
					mysql_free_result(resultado);
					char Pesquisar_ADMS[250];
					snprintf(Pesquisar_ADMS,sizeof(Pesquisar_ADMS),"SELECT nome_usuario FROM usuarios_admins");
					if(mysql_query(conexao,Pesquisar_ADMS)==0)
					{
						resultado = mysql_store_result(conexao);
						if(resultado != NULL && mysql_num_rows(resultado)>0)
						{
							while(row = mysql_fetch_row(resultado))
							{
								char Pendencia[250];
								snprintf(Pendencia,sizeof(Pendencia),"SELECT COUNT(lida) FROM bate_papo WHERE lida = FALSE AND nome_do_remetente = '%s' AND nome_do_destinatario = '%s'",row[0],Nome);
								if(mysql_query(conexao,Pendencia)==0)
								{
									outro_resultado=mysql_store_result(conexao);
									if(outro_resultado != NULL && mysql_num_rows(outro_resultado)>0)
									{
										while(coluna = mysql_fetch_row(outro_resultado))
										{
											if(g_strcmp0(coluna[0],"0")!= 0)
											{
												char naovistas[250];
												snprintf(naovistas,sizeof(naovistas),"Mensagens nao vistas : %s",coluna[0]);
												gtk_list_store_append(list11, &iter);
												gtk_list_store_set(list11, &iter, 0,row[0], 1, naovistas, -1);
											}
										}

									}

									mysql_free_result(outro_resultado);
								}
							}
						}
						mysql_free_result(resultado);
					}
				}
			}
		}
	}
	mysql_close(conexao);
}


void on_Abrir_Chat(GtkButton *button, gpointer user_data)
{

	/* */

	int x = 0;
	CarregarLidas(x);

	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "selecionar_empresa1"));
	gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
	gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
	gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
	gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
	GtkListStore *list11 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore11"));
	gtk_list_store_clear(list11);


	/* */
}

void on_Enviar_Mensagem(GtkButton *button, gpointer user_data)
{
	char data_hora[20];
	time_t tempo_atual;
	time(&tempo_atual);
	struct tm *data_hora_local = localtime(&tempo_atual);
	sprintf(data_hora, "%04d-%02d-%02d %02d:%02d:%02d",
	        data_hora_local->tm_year + 1900,
	        data_hora_local->tm_mon + 1,
	        data_hora_local->tm_mday,
	        data_hora_local->tm_hour,
	        data_hora_local->tm_min,
	        data_hora_local->tm_sec);

	char hora[20];
	time_t tempo_atual2;
	time(&tempo_atual2);
	struct tm *data_hora_local2 = localtime(&tempo_atual2);
	sprintf(hora, "%02d:%02d",
	        data_hora_local2->tm_hour,
	        data_hora_local2->tm_min);

	MYSQL *conexao = obterConexao();
	if (conexao == NULL)
	{
		mensagem("Erro", "Não foi possível conectar ao banco de dados", 0, "Erro");
		return;
	}

	gchar *Mensagem = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(user_data, "Mensagem_a_ser_enviada")));
	gchar *Remetente = gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa")));
	gchar *valor_Resposta = gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
	GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(user_data, "liststore10"));
	GtkTreeIter iter;

	char Pesquisa_Usuario_Atual[250], PesquisaUsuario_Atual_ADM[250];
	char id_de_usuario[7] = "";
	char id_de_remetente[7] = "";
	MYSQL_RES *resultado = NULL;
	MYSQL_ROW row;

	snprintf(Pesquisa_Usuario_Atual, sizeof(Pesquisa_Usuario_Atual),
	         "SELECT id_usuario_empresa FROM usuarios_empresas WHERE razao_social = '%s'", Nome);
	snprintf(PesquisaUsuario_Atual_ADM, sizeof(PesquisaUsuario_Atual_ADM),
	         "SELECT id_usuario_adm FROM usuarios_admins WHERE nome_usuario = '%s'", Nome);

	if (mysql_query(conexao, Pesquisa_Usuario_Atual) == 0)
	{

		resultado = mysql_store_result(conexao);
		if(resultado!=NULL && mysql_num_rows(resultado) > 0)
		{

			if (resultado && (row = mysql_fetch_row(resultado)))
			{
				strncpy(id_de_usuario, row[0], sizeof(id_de_usuario) - 1);
			}
		}
		else
		{
			mysql_free_result(resultado);
			if (mysql_query(conexao, PesquisaUsuario_Atual_ADM) == 0)
			{
				resultado = mysql_store_result(conexao);
				if(resultado!=NULL && mysql_num_rows(resultado) > 0)
				{


					if (resultado && (row = mysql_fetch_row(resultado)))
					{
						strncpy(id_de_usuario, row[0], sizeof(id_de_usuario) - 1);
					}
				}
			}
		}
		mysql_free_result(resultado);
	}
	else if (mysql_query(conexao, PesquisaUsuario_Atual_ADM) == 0)
	{
		resultado = mysql_store_result(conexao);
		if(resultado!=NULL && mysql_num_rows(resultado) > 0)
		{

			if (resultado && (row = mysql_fetch_row(resultado)))
			{
				strncpy(id_de_usuario, row[0], sizeof(id_de_usuario) - 1);
			}
		}
		mysql_free_result(resultado);
	}

	if (g_strcmp0(id_de_usuario, "") == 0)
	{
		mensagem("Erro", "Usuário não encontrado", 0, "Erro");
		return;
	}

	if (g_strcmp0(Remetente, "") == 0)
	{
		mensagem("Erro", "Nenhum Remetente Inserido", 0, "Erro");
		return;
	}

	if (g_strcmp0(Mensagem, "") == 0)
	{
		mensagem("Erro", "Digite uma Mensagem", 0, "Erro");
		return;
	}

	char PesquisaRemetente[250], DadosMensagem[500],PesquisaRemetenteADM[250];
	snprintf(PesquisaRemetente, sizeof(PesquisaRemetente),
	         "SELECT id_usuario_empresa FROM usuarios_empresas WHERE razao_social = '%s'", Remetente);

	if (mysql_query(conexao, PesquisaRemetente) == 0)
	{
		resultado = mysql_store_result(conexao);

		if(resultado!=NULL && mysql_num_rows(resultado) > 0)
		{


			if (resultado && (row = mysql_fetch_row(resultado)))
			{
				strncpy(id_de_remetente, row[0], sizeof(id_de_remetente) - 1);
			}
			mysql_free_result(resultado);
		}
		else
		{
			mysql_free_result(resultado);

			snprintf(PesquisaRemetenteADM, sizeof(PesquisaRemetenteADM),
			         "SELECT id_usuario_adm FROM usuarios_admins WHERE nome_usuario = '%s'", Remetente);
			if (mysql_query(conexao, PesquisaRemetenteADM) == 0)
			{
				resultado = mysql_store_result(conexao);
				if(resultado!=NULL && mysql_num_rows(resultado) > 0)
				{
					if (resultado && (row = mysql_fetch_row(resultado)))
					{
						strncpy(id_de_remetente, row[0], sizeof(id_de_remetente) - 1);
					}
				}
				mysql_free_result(resultado);
			}
		}

	}
	else
	{
		snprintf(PesquisaRemetenteADM, sizeof(PesquisaRemetenteADM),
		         "SELECT id_usuario_adm FROM usuarios_admins WHERE nome_usuario = '%s'", Remetente);
		if (mysql_query(conexao, PesquisaRemetenteADM) == 0)
		{
			resultado = mysql_store_result(conexao);
			if(resultado!=NULL && mysql_num_rows(resultado) > 0)
			{
				if (resultado && (row = mysql_fetch_row(resultado)))
				{
					strncpy(id_de_remetente, row[0], sizeof(id_de_remetente) - 1);
				}
			}
			mysql_free_result(resultado);
		}
	}

	if (g_strcmp0(id_de_remetente, "") == 0)
	{
		mensagem("Erro", "Remetente inexistente", 0, "Erro");
		return;
	}

	if (g_strcmp0(valor_Resposta, "") == 0)
	{
		snprintf(DadosMensagem, sizeof(DadosMensagem),
		         "INSERT INTO bate_papo(remetente_id, destinatario_id, conteudo, hora_envio, nome_do_remetente, nome_do_destinatario) "
		         "VALUES ('%s', '%s', '%s', '%s','%s','%s')", id_de_usuario, id_de_remetente, Mensagem, data_hora,Nome,Remetente);

		CarregarChat();
		GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
		gtk_label_set_text(Mensagem,"");
		GtkEntry *Mensagem_valor = (GTK_ENTRY(gtk_builder_get_object(user_data, "Mensagem_a_ser_enviada")));
		gtk_entry_set_text(Mensagem_valor,"");
	}
	else
	{
		snprintf(DadosMensagem, sizeof(DadosMensagem),
		         "INSERT INTO bate_papo(remetente_id, destinatario_id, conteudo, mensagem_referente, hora_envio, nome_do_remetente, nome_do_destinatario) "
		         "VALUES ('%s', '%s', '%s', '%s', '%s','%s','%s')", id_de_usuario, id_de_remetente, Mensagem, valor_Resposta, data_hora,Nome,Remetente);

		CarregarChat();
		GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
		gtk_label_set_text(Mensagem,"");
		GtkEntry *Mensagem_valor =  (GTK_ENTRY(gtk_builder_get_object(user_data, "Mensagem_a_ser_enviada")));
		gtk_entry_set_text(Mensagem_valor,"");

	}

	if (mysql_query(conexao, DadosMensagem) != 0)
	{
		mensagem("Erro", "Erro no envio da mensagem", 0, "Erro");
	}
	mysql_close(conexao);
}



void on_Chat_clicked(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *valor_data;
	model = gtk_tree_view_get_model(treeview);

	// Obtém o iterador da linha clicada
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		if (g_strcmp0(gtk_tree_view_column_get_title(column), "Suas Mensagens") == 0)
		{
			gtk_tree_model_get(model, &iter, 1, &valor_data, -1);
		}
		else if (g_strcmp0(gtk_tree_view_column_get_title(column), "Mensagens") == 0)
		{
			gtk_tree_model_get(model, &iter, 0, &valor_data, -1);
		}

		// Verifica se valor_data não é nulo
		if (valor_data != NULL)
		{
			gchar mensagem_formatada[50]; // Buffer para a mensagem formatada
			formatar_mensagem(valor_data, mensagem_formatada, sizeof(mensagem_formatada));

			GtkLabel *Mensagem = GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao"));
			gtk_label_set_text(Mensagem, mensagem_formatada);
		}
	}
}

void on_Apagar_Respondendo_a(GtkButton *button, gpointer user_data)
{
	GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Mensagem_em_questao")));
	gtk_label_set_text(Mensagem,"");
}

void on_Sair_da_escolha_mensagem(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "selecionar_empresa1"));
	GtkListStore *list11 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore11"));
	gtk_list_store_clear(list11);
	gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));

}

void on_Pesquisar_Usuarios(GtkEntry *entry,gpointer user_data)
{
	int x = 1;
	CarregarLidas(x);
}

void on_Filtrar_não_lidas(GtkToggleButton *toggle_button, gpointer user_data)
{
	gboolean is_active = gtk_toggle_button_get_active(toggle_button);
	if (is_active)
	{
		int x = 2;
		CarregarLidas(x);
	}
	else
	{
		int x=1;
		CarregarLidas(x);
	}

}
void on_Iniciar_Chat_empresa(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *valor_data;
	model = gtk_tree_view_get_model(treeview);

	// Obtém o iterador da linha clicada
	if (gtk_tree_model_get_iter(model, &iter, path))
	{

		gtk_tree_model_get(model, &iter, 0, &valor_data, -1);
		char NomesSelecionado[250];
		snprintf(NomesSelecionado,sizeof(NomesSelecionado),"%s",valor_data);

		GtkLabel *Mensagem = (GTK_LABEL(gtk_builder_get_object(user_data, "Nome_Empresa")));
		gtk_label_set_text(Mensagem,NomesSelecionado);




		CarregarChat();
		// Agende a função para ser chamada a cada 5000 milissegundos (5 segundos)
		carregar_chat = g_timeout_add_seconds(1, (GSourceFunc)Carregar_Chat_Segundos, (gpointer)builder);



		GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "BatePapo"));
		gtk_widget_queue_draw(GTK_WIDGET(mensagem_dialogo));
		gtk_widget_show_all(GTK_WIDGET(mensagem_dialogo));
		gtk_dialog_run(GTK_DIALOG(mensagem_dialogo));
		gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));

		int x = 0;
		GtkListStore *list11 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore11"));
		gtk_list_store_clear(list11);


		GtkToggleButton *Filtrar_não_lidas = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "Filtrar_não_lidas"));
		gtk_toggle_button_set_active(Filtrar_não_lidas,FALSE);

		GtkEntry *Entrada = (GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_Usuarios")));
		gtk_entry_set_text(Entrada,"");
		CarregarLidas(x);
		g_source_remove(carregar_chat);
		carregar_chat = 0;
		GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
		gtk_list_store_clear(list10);
		snprintf(hora_mensagem,sizeof(hora_mensagem),"0");



	}

}
void on_Cancelar_Seleção1(GtkButton *button, gpointer user_data)
{
	GtkMessageDialog *mensagem_dialogo = GTK_MESSAGE_DIALOG(gtk_builder_get_object(user_data, "BatePapo"));

	gtk_widget_hide(GTK_WIDGET(mensagem_dialogo));
	// Agende a função para ser chamada a cada 5000 milissegundos (5 segundos)
	int x = 0;
	GtkToggleButton *Filtrar_não_lidas = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "Filtrar_não_lidas"));
	gtk_toggle_button_set_active(Filtrar_não_lidas,FALSE);

	GtkEntry *Entrada = (GTK_ENTRY(gtk_builder_get_object(user_data, "Pesquisar_Usuarios")));
	gtk_entry_set_text(Entrada,"");

	GtkListStore *list11 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore11"));
	gtk_list_store_clear(list11);

	g_source_remove(carregar_chat);
	CarregarLidas(x);
	carregar_chat = 0;
	GtkListStore *list10 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore10"));
	gtk_list_store_clear(list10);
	snprintf(hora_mensagem,sizeof(hora_mensagem),"0");
}

void on_Chamados_nao_lidos_filtrar(GtkToggleButton *toggle_button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES  *lista3 = NULL;
	MYSQL_ROW row;

	GtkListStore *store3 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore3"));
	char list3[250];
	gtk_list_store_clear(store3);
	gboolean is_active = gtk_toggle_button_get_active(toggle_button);
	if (is_active)
	{
		snprintf(list3, sizeof(list3), "SELECT titulo_chamado, corpo_chamado, nome_empresa, data_criacao, id_chamado FROM chamados WHERE lido = 0 ORDER BY id_chamado DESC");
		if (mysql_query(conexao, list3) == 0)
		{
			lista3 = mysql_store_result(conexao);
		}

		if (lista3 != NULL && mysql_num_rows(lista3) > 0)
		{

			while ((row = mysql_fetch_row(lista3)) != NULL)
			{

				GtkTreeIter list3_iter;
				char titulo[700];
				snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
				gtk_list_store_append(store3, &list3_iter);
				gtk_list_store_set(store3, &list3_iter, 0, titulo, -1);


				gtk_list_store_append(store3, &list3_iter);
				gtk_list_store_set(store3, &list3_iter, 0, row[1],-1);

				char empresa[200];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(empresa, sizeof(empresa), "Empresa Referente: %s ",row[2]);
				gtk_list_store_set(store3, &list3_iter, 0, empresa,-1);

				char data[200];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
				gtk_list_store_set(store3, &list3_iter, 0, data,-1);

				char divisoria[800];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
				gtk_list_store_set(store3, &list3_iter, 0, divisoria,-1);

			}
			mysql_free_result(lista3);
		}
	}
	else
	{
		snprintf(list3, sizeof(list3), "SELECT titulo_chamado, corpo_chamado, nome_empresa, data_criacao, id_chamado FROM chamados ORDER BY id_chamado DESC");
		if (mysql_query(conexao, list3) == 0)
		{
			lista3 = mysql_store_result(conexao);
		}

		if (lista3 != NULL && mysql_num_rows(lista3) > 0)
		{
			int x=0;
			while ((row = mysql_fetch_row(lista3)) != NULL)
			{


				GtkTreeIter list3_iter;
				char titulo[700];
				snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
				gtk_list_store_append(store3, &list3_iter);
				gtk_list_store_set(store3, &list3_iter, 0, titulo, -1);


				gtk_list_store_append(store3, &list3_iter);
				gtk_list_store_set(store3, &list3_iter, 0, row[1],-1);

				char empresa[200];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(empresa, sizeof(empresa), "Empresa Referente: %s ",row[2]);
				gtk_list_store_set(store3, &list3_iter, 0, empresa,-1);

				char data[200];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
				gtk_list_store_set(store3, &list3_iter, 0, data,-1);

				char divisoria[800];
				gtk_list_store_append(store3, &list3_iter);
				snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
				gtk_list_store_set(store3, &list3_iter, 0, divisoria,-1);

			}
			mysql_free_result(lista3);
		}
	}
	mysql_close(conexao);
}
void on_Mensagens_nao_lidas_filtrar(GtkToggleButton *toggle_button, gpointer user_data)
{
	MYSQL *conexao = obterConexao();
	MYSQL_RES  *lista3 = NULL;
	MYSQL_ROW row;

	GtkListStore *store2 = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore2"));
	char list3[250];
	gtk_list_store_clear(store2);

	gboolean is_active = gtk_toggle_button_get_active(toggle_button);

	if (is_active)
	{

		snprintf(list3, sizeof(list3), "SELECT titulo_mensagem, corpo_mensagem, usuario_criador, data_de_criacao, id_mensagem FROM mensagens WHERE empresa_destino = '%s' AND lido = FALSE ORDER BY id_mensagem DESC",Nome);


		if (mysql_query(conexao, list3) == 0)
		{
			lista3 = mysql_store_result(conexao);
		}

		if (lista3 != NULL && mysql_num_rows(lista3) > 0)
		{
			int x=0;
			while ((row = mysql_fetch_row(lista3)) != NULL)
			{

				GtkTreeIter list2_iter;
				char titulo[700];
				snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
				gtk_list_store_append(store2, &list2_iter);
				gtk_list_store_set(store2, &list2_iter, 0, titulo, -1);


				gtk_list_store_append(store2, &list2_iter);
				gtk_list_store_set(store2, &list2_iter, 0, row[1],-1);

				char empresa[200];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(empresa, sizeof(empresa), "Criado por: %s ",row[2]);
				gtk_list_store_set(store2, &list2_iter, 0, empresa,-1);

				char data[200];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
				gtk_list_store_set(store2, &list2_iter, 0, data,-1);

				char divisoria[800];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
				gtk_list_store_set(store2, &list2_iter, 0, divisoria,-1);

			}
			mysql_free_result(lista3);
		}
	}
	else
	{

		snprintf(list3, sizeof(list3), "SELECT titulo_mensagem, corpo_mensagem, usuario_criador, data_de_criacao, id_mensagem FROM mensagens WHERE empresa_destino = '%s' ORDER BY id_mensagem DESC",Nome);


		if (mysql_query(conexao, list3) == 0)
		{
			lista3 = mysql_store_result(conexao);
		}

		if (lista3 != NULL && mysql_num_rows(lista3) > 0)
		{
			int x=0;
			while ((row = mysql_fetch_row(lista3)) != NULL)
			{

				GtkTreeIter list2_iter;
				char titulo[700];
				snprintf(titulo,sizeof(titulo),"Cabeçalho : %s",row[0]);
				gtk_list_store_append(store2, &list2_iter);
				gtk_list_store_set(store2, &list2_iter, 0, titulo, -1);


				gtk_list_store_append(store2, &list2_iter);
				gtk_list_store_set(store2, &list2_iter, 0, row[1],-1);

				char empresa[200];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(empresa, sizeof(empresa), "Criado por: %s ",row[2]);
				gtk_list_store_set(store2, &list2_iter, 0, empresa,-1);

				char data[200];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(data, sizeof(data), "Data de Criação: %s Para Responder Clique Aqui!!",row[3]);
				gtk_list_store_set(store2, &list2_iter, 0, data,-1);

				char divisoria[800];
				gtk_list_store_append(store2, &list2_iter);
				snprintf(divisoria, sizeof(divisoria), "_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________");
				gtk_list_store_set(store2, &list2_iter, 0, divisoria,-1);

			}
			mysql_free_result(lista3);
		}
	}
	mysql_close(conexao);
}

//-------------------------------------------------------------------------------Funcao Main------------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	snprintf(hora_mensagem,sizeof(hora_mensagem),"0");
	snprintf(id_chamado,sizeof(id_chamado),"0");
	snprintf(id_mensagem,sizeof(id_mensagem),"0");
	snprintf(id_relatorioEmpresa,sizeof(id_relatorioEmpresa),"0");
	snprintf(id_relatorioADM,sizeof(id_relatorioADM),"0");
//  variavel para conexão com banco
	MYSQL *conexao = obterConexao();

	GtkWidget *window;
	GtkStack *stack;

	// Inicializa GTK
	gtk_init(&argc, &argv);

	// Carrega o CSS
	carregar_css();

	// Carrega o XML da interface
	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, "./glade/apk.glade", NULL))
	{
		g_printerr("Erro ao carregar arquivo de interface XML.\n");
		return 1;
	}



	// Obtém a janela principal
	window = GTK_WIDGET(gtk_builder_get_object(builder, "Janela_Principal"));

	// Obtém o GtkStack para trocar de página
	stack = GTK_STACK(gtk_builder_get_object(builder, "Telas"));

	// Conecta o botão de login ao evento de troca de página
	GtkWidget *botao_login = GTK_WIDGET(gtk_builder_get_object(builder, "Botao_Login"));
	g_signal_connect(botao_login, "clicked", G_CALLBACK(on_Botao_Login_clicked), stack);

	// Conecta o botão de logout ao evento de retorno para a página Iniciar
	GtkWidget *botao_logout = GTK_WIDGET(gtk_builder_get_object(builder, "BotaoVoltar"));
	g_signal_connect(botao_logout, "clicked", G_CALLBACK(on_BotaoVoltar_clicked), stack);



	//Botao Logout Empresa

	// Conecta o botão de logout ao evento de retorno para a página Iniciar
	GtkWidget *botao_sair = GTK_WIDGET(gtk_builder_get_object(builder, "Efetuar_Logout"));
	g_signal_connect(botao_sair, "clicked", G_CALLBACK(on_Efetuar_Logout_clicked), builder);

	GtkWidget *Login = GTK_WIDGET(gtk_builder_get_object(builder, "Login_Label"));
	g_signal_connect(Login, "button-press-event", G_CALLBACK(on_Login_clicked), stack);

	//Botao Logout ADM

	GtkWidget *botao_sair_adm = GTK_WIDGET(gtk_builder_get_object(builder, "Efetuar_Logout_Adm"));
	g_signal_connect(botao_sair_adm, "clicked", G_CALLBACK(on_Efetuar_Logout_clicked), builder);


	// Conecta o botão Enviar ao evento
	GtkWidget *logar = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar"));
	g_signal_connect(logar, "clicked", G_CALLBACK(on_Enviar_clicked), builder);  // Passa o builder como user_data

	GtkWidget *saircaixa = GTK_WIDGET(gtk_builder_get_object(builder, "botao_sair_caixa_mensagem"));
	g_signal_connect(saircaixa, "clicked", G_CALLBACK(on_botao_sair_caixa_mensagem), builder);
	// Conecta o sinal destroy para fechar a janela
	g_signal_connect(window, "destroy", G_CALLBACK(on_Janela_Principal_destroy), NULL);

	GtkWidget *botao_cadastrar_usuario = GTK_WIDGET(gtk_builder_get_object(builder, "Cadastrar_Usuario"));
	g_signal_connect(botao_cadastrar_usuario, "clicked", G_CALLBACK(on_Cadastrar_Usuario_clicked), builder);




//--------------------------------------------------------------Botoes Cadastro de Usuario-------------------------------------------------------------------------------------------------------------


	GtkWidget *FecharCadastro = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Envio"));
	g_signal_connect(FecharCadastro, "clicked", G_CALLBACK(on_Cancelar_Envio_clicked), builder);

	GtkWidget *FecharCadastroadm = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Envio_adm"));
	g_signal_connect(FecharCadastroadm, "clicked", G_CALLBACK(on_Cancelar_Envio_clicked), builder);

	GtkWidget *Caixa_de_troca = GTK_WIDGET(gtk_builder_get_object(builder, "Caixa_Desativa"));
	g_signal_connect(Caixa_de_troca, "state-set", G_CALLBACK(on_Caixa_Desativa_activate), builder);

	GtkWidget *Enviar_Dados_Empresa = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Dados_Usuario_Empresa"));
	g_signal_connect(Enviar_Dados_Empresa, "clicked", G_CALLBACK(on_Enviar_Dados_Usuario_Empresa), builder);

	GtkWidget *Enviar_Dados_ADM = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Dados_Usuario_ADM"));
	g_signal_connect(Enviar_Dados_ADM, "clicked", G_CALLBACK(on_Enviar_Dados_Usuario_ADM), builder);



//----------------------------------------------------------------------------Cadastro de Relatorios--------------------------------------------------------------------------------------------------------------------

	GtkWidget *Fechar_aba_relatorio = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Relatorio"));
	g_signal_connect(Fechar_aba_relatorio, "clicked", G_CALLBACK(on_Cancelar_Relatorio), builder);

	GtkWidget *Abrir_criacao_relatorios = GTK_WIDGET(gtk_builder_get_object(builder, "Gerar Relatorio"));
	g_signal_connect(Abrir_criacao_relatorios, "clicked", G_CALLBACK(on_Gerar_Relatorio), builder);

	GtkWidget *Selecionar_empresa = GTK_WIDGET(gtk_builder_get_object(builder, "Selecionar_Empresa"));
	g_signal_connect(Selecionar_empresa, "clicked", G_CALLBACK(on_selecionar_empresa), builder);

	GtkWidget *Enviar_Relatorio  = GTK_WIDGET(gtk_builder_get_object(builder, "Salvar_Relatorio"));
	g_signal_connect(Enviar_Relatorio, "clicked", G_CALLBACK(on_enviar_relatorio), builder);

	GtkEntry *CEP_empresa = (GTK_ENTRY(gtk_builder_get_object(builder, "CEP_empresa")));
	g_signal_connect(CEP_empresa, "changed", G_CALLBACK(buscar_endereco_por_cep), builder);


//------------------------------------------------------------------------Eventos Clique Tabela Relatorios-------------------------------------------------------------------------------------------------------------------
	GtkTreeView *listaEmpresas = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Coluna_empresas"));
	GtkTreeView *Tabela_Empresa = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Tabela_empresa"));
	GtkTreeView *Tabela_ADM = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Relatorios_ADM"));
	GtkEntry *pesquisar_relatorio = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_Empresa")));
	g_signal_connect(pesquisar_relatorio, "search-changed", G_CALLBACK(on_pesquisar_relatorio_entry_changed), builder);


	g_signal_connect(listaEmpresas, "row-activated", G_CALLBACK(on_row_activated), builder);
	g_signal_connect(Tabela_ADM, "row-activated", G_CALLBACK(on_Relatorios_ADM_row_activated), builder);
	g_signal_connect(Tabela_Empresa, "row-activated", G_CALLBACK(on_Tabelo_empresa_row_activated), builder);


//-----------------------------------------------------------------------------Botão Caixa para escolher Pasta---------------------------------------------------------------------------------------------------------------

	GtkWidget *file_chooser_dialog = GTK_WIDGET(gtk_builder_get_object(builder,"Escolher_local_para_dowload"));
	g_signal_connect(gtk_builder_get_object(builder, "Selecionar_pasta"), "clicked", G_CALLBACK(on_Selecionar_pasta_clicked), file_chooser_dialog);
	g_signal_connect(gtk_builder_get_object(builder, "Cancelar_Busca"), "clicked", G_CALLBACK(on_Cancelar_Busca_clicked), file_chooser_dialog);
	gtk_widget_hide(file_chooser_dialog);


//---------------------------------------------------------------------------------Caixa de Mensagens------------------------------------------------------------------------------------------------------------------------

	GtkWidget *Abrir_caixa_de_mensagens = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Mensagem"));
	g_signal_connect(Abrir_caixa_de_mensagens, "clicked", G_CALLBACK(on_abrir_caixa_mensagem), builder);  // Passa o builder como user_data


	GtkWidget *Fechar_caixa_de_mensagens = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Mensagem"));
	g_signal_connect(Fechar_caixa_de_mensagens, "clicked", G_CALLBACK(on_sair_caixa_de_mensagem), builder);


	GtkWidget *Abrir_Selecao_de_empresas = GTK_WIDGET(gtk_builder_get_object(builder, "Abrir_Selecao_de_empresas"));
	g_signal_connect(Abrir_Selecao_de_empresas, "clicked", G_CALLBACK(on_Abrir_Selecao_de_empresas), builder);

	GtkWidget *Selecionar_todas_empresas = GTK_WIDGET(gtk_builder_get_object(builder, "Selecionar_Todos"));
	g_signal_connect(Selecionar_todas_empresas, "toggled", G_CALLBACK(on_Selecionar_Todas), builder);

	GtkWidget *Enviar_Selecionados = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Selecionados"));
	g_signal_connect(Enviar_Selecionados, "clicked", G_CALLBACK(on_Enviar_Selecionados), builder);

	GtkWidget *Fechar_Selecao = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar Seleção"));
	g_signal_connect(Fechar_Selecao, "clicked", G_CALLBACK(on_Fechar_Selecao), builder);

	GtkWidget *Apagar_Selecionados = GTK_WIDGET(gtk_builder_get_object(builder, "Apagar_Selecionados"));
	g_signal_connect(Apagar_Selecionados, "clicked", G_CALLBACK(on_Apagar_Selecionados), builder);

	GtkTreeView *Retira_selecionado = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Clique_Apaga"));
	GtkTreeView *Clique_Seleciona = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Selecao"));

	g_signal_connect(Retira_selecionado, "row-activated", G_CALLBACK(on_Retira_selecionado_activated), builder);
	g_signal_connect(Clique_Seleciona, "row-activated", G_CALLBACK(on_Clique_Seleciona_activated), builder);

	GtkEntry *pesquisar = (GTK_ENTRY(gtk_builder_get_object(builder, "pesquisar_empresas")));

	g_signal_connect(pesquisar, "search-changed", G_CALLBACK(on_search_entry_changed), builder);

	GtkWidget *Enviar_Dados_Mensagem = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Dados_Mensagem"));
	g_signal_connect(Enviar_Dados_Mensagem, "clicked", G_CALLBACK(on_Enviar_Dados_Mensagem), builder);

//-------------------------------------------------------------------------------------Botao de abertura de chamado---------------------------------------------------------------------------------------------------------------


	GtkWidget *Abrir_chamado = GTK_WIDGET(gtk_builder_get_object(builder, "Abrir_Chamado"));
	g_signal_connect(Abrir_chamado, "clicked", G_CALLBACK(on_abrir_chamado_janela), builder);

	GtkWidget *Cancelar_chamado = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Chamado"));
	g_signal_connect(Cancelar_chamado, "clicked", G_CALLBACK(on_Cancelar_chamado), builder);


	GtkWidget *Enviar_Chamado = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Dados_Chamado"));
	g_signal_connect(Enviar_Chamado, "clicked", G_CALLBACK(on_Enviar_Chamado), builder);


//=--------------------------------------------------------------------------------------Botao de Historico de Mensagens---------------------------------------------------------------------------------------------------------


	GtkWidget *Abrir_caixa_historico = GTK_WIDGET(gtk_builder_get_object(builder, "Historico_de_mensagens"));
	g_signal_connect(Abrir_caixa_historico, "clicked", G_CALLBACK(on_Abrir_historico), builder);


	GtkWidget *Cancelar_Envio_das_edicoes = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Envio_das_edicoes"));
	g_signal_connect(Cancelar_Envio_das_edicoes, "clicked", G_CALLBACK(on_Cancelar_Envio_das_edicoes), builder);


	GtkTreeView *Editar_Mensagem_on = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Editar_Mensagem_on"));
	g_signal_connect(Editar_Mensagem_on, "row-activated", G_CALLBACK(on_Editar_Mensagem_on), builder);

	GtkWidget *Cancelar_Edicao = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Edicao"));
	g_signal_connect(Cancelar_Edicao, "clicked", G_CALLBACK(on_Cancelar_Edicao), builder);


	GtkWidget *Enviar_Edicao = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Edicao"));
	g_signal_connect(Enviar_Edicao, "clicked", G_CALLBACK(on_Enviar_Edicao), builder);

	GtkWidget *cancela_Envio = GTK_WIDGET(gtk_builder_get_object(builder, "cancela_Envio"));
	g_signal_connect(cancela_Envio, "clicked", G_CALLBACK(on_cancela_Envio), builder);

	GtkWidget *Confirmar_envio_mensagem = GTK_WIDGET(gtk_builder_get_object(builder, "Confirmar_envio_mensagem"));
	g_signal_connect(Confirmar_envio_mensagem, "clicked", G_CALLBACK(on_Confirmar_envio_mensagem), builder);

//=--------------------------------------------------------------------------------------Botao de Historico de Chamados---------------------------------------------------------------------------------------------------------


	GtkWidget *Abrir_Historico_de_Chamados = GTK_WIDGET(gtk_builder_get_object(builder, "Abrir_Historico_de_Chamados"));
	g_signal_connect(Abrir_Historico_de_Chamados, "clicked", G_CALLBACK(on_Abrir_Historico_de_Chamados), builder);


	GtkWidget *Cancelar_Envio_dos_chamados = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Envio_dos_chamados"));
	g_signal_connect(Cancelar_Envio_dos_chamados, "clicked", G_CALLBACK(on_Cancelar_Envio_dos_chamados), builder);


	GtkTreeView *Editar_Chamado_on = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Editar_Chamado_on"));
	g_signal_connect(Editar_Chamado_on, "row-activated", G_CALLBACK(on_Editar_Chamado_on), builder);

	GtkWidget *Cancelar_Envio_Chamado = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar_Envio_Chamado"));
	g_signal_connect(Cancelar_Envio_Chamado, "clicked", G_CALLBACK(on_Cancelar_Envio_Chamado), builder);


	GtkWidget *Enviar_Chamado_editado = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Chamado_editado"));
	g_signal_connect(Enviar_Chamado_editado, "clicked", G_CALLBACK(on_Enviar_Chamado_editado), builder);

	GtkWidget *cancela_Chamado = GTK_WIDGET(gtk_builder_get_object(builder, "cancela_Chamado"));
	g_signal_connect(cancela_Chamado, "clicked", G_CALLBACK(on_cancela_Chamado), builder);

	GtkWidget *Confirmar_envio_Chamado = GTK_WIDGET(gtk_builder_get_object(builder, "Confirmar_envio_Chamado"));
	g_signal_connect(Confirmar_envio_Chamado, "clicked", G_CALLBACK(on_Confirmar_envio_Chamado), builder);

//------------------------------------------------------------------------------------Relatorio Detalhado---------------------------------------------------------------------------------------------------------------------

	GtkWidget *Relatorio_Detalhado = GTK_WIDGET(gtk_builder_get_object(builder, "Relatorio_Detalhado"));
	g_signal_connect(Relatorio_Detalhado, "button-press-event", G_CALLBACK(on_Relatorio_Detalhado), builder);

	GtkWidget *Relatorios_Detalhado_de_empresa = GTK_WIDGET(gtk_builder_get_object(builder, "Relatorios_Detalhado_de_empresa"));
	g_signal_connect(Relatorios_Detalhado_de_empresa, "button-press-event", G_CALLBACK(on_Relatorio_Detalhado), builder);

	GtkWidget *Sair_Relatorio_Detalhado = GTK_WIDGET(gtk_builder_get_object(builder, "Sair_Relatorio_Detalhado"));
	g_signal_connect(Sair_Relatorio_Detalhado, "clicked", G_CALLBACK(on_Sair_Relatorio_Detalhado), builder);

	GtkWidget *Sair_Relatorio_Detalhado1 = GTK_WIDGET(gtk_builder_get_object(builder, "Sair_Relatorio_Detalhado1"));
	g_signal_connect(Sair_Relatorio_Detalhado1, "clicked", G_CALLBACK(on_Sair_Relatorio_Detalhado1), builder);

	GtkWidget *Fazer_Pesquisa_Entre_datas1 = GTK_WIDGET(gtk_builder_get_object(builder, "Fazer_Pesquisa_Entre_datas1"));
	g_signal_connect(Fazer_Pesquisa_Entre_datas1, "clicked", G_CALLBACK(on_Fazer_Pesquisa_Entre_datas1), builder);


	GtkWidget *Fazer_Pesquisa_Entre_datas = GTK_WIDGET(gtk_builder_get_object(builder, "Fazer_Pesquisa_Entre_datas"));
	g_signal_connect(Fazer_Pesquisa_Entre_datas, "clicked", G_CALLBACK(on_Fazer_Pesquisa_Entre_datas), builder);

	GtkWidget *Gerar_CSV = GTK_WIDGET(gtk_builder_get_object(builder, "Gerar_CSV"));
	g_signal_connect(Gerar_CSV, "button-press-event", G_CALLBACK(on_Gerar_CSV), builder);

	GtkWidget *Gerar_CSV_Empresa = GTK_WIDGET(gtk_builder_get_object(builder, "Gerar_CSV_Empresa"));
	g_signal_connect(Gerar_CSV_Empresa, "button-press-event", G_CALLBACK(on_Gerar_CSV), builder);

//------------------------------------------------------------------------------------Pesquisas Relatorio Geral----------------------------------------------------------------------------------------------------------------

	GtkEntry *Pesquisar_por_Data = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_por_Data")));
	g_signal_connect(Pesquisar_por_Data, "search-changed", G_CALLBACK(on_pesquisar_relatorios), builder);

	GtkEntry *Pesquisar_por_nome_empresa = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_por_nome_empresa")));
	g_signal_connect(Pesquisar_por_nome_empresa, "search-changed", G_CALLBACK(on_pesquisar_relatorios), builder);

	GtkEntry *Pesquisar_por_nome_arquivo = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_por_nome_arquivo")));
	g_signal_connect(Pesquisar_por_nome_arquivo, "search-changed", G_CALLBACK(on_pesquisar_relatorios), builder);

	GtkEntry *Pesquisar_por_Data1 = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_por_Data1")));
	g_signal_connect(Pesquisar_por_Data1, "search-changed", G_CALLBACK(on_pesquisar_relatorio_empresas), builder);

	GtkEntry *Pesquisar_por_nome_arquivo1 = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_por_nome_arquivo1")));
	g_signal_connect(Pesquisar_por_nome_arquivo1, "search-changed", G_CALLBACK(on_pesquisar_relatorio_empresas), builder);


//------------------------------------------------------------------------------Bate Papo e Respostas de mensagens e chamados-----------------------------------------------------------------------------------------------------

	GtkTreeView *Tabela_Chamados = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Tabela_Chamados"));
	g_signal_connect(Tabela_Chamados, "row-activated", G_CALLBACK(on_Tabela_Chamados_responder), builder);


	GtkWidget *Iniciar_Chat = GTK_WIDGET(gtk_builder_get_object(builder, "Iniciar_Chat"));
	g_signal_connect(Iniciar_Chat, "clicked", G_CALLBACK(on_Abrir_Chat), builder);

	GtkWidget *Iniciar_Chat1 = GTK_WIDGET(gtk_builder_get_object(builder, "Iniciar_Chat1"));
	g_signal_connect(Iniciar_Chat1, "clicked", G_CALLBACK(on_Abrir_Chat), builder);


	GtkWidget *Enviar_Mesagem = GTK_WIDGET(gtk_builder_get_object(builder, "Enviar_Mesagem"));
	g_signal_connect(Enviar_Mesagem, "clicked", G_CALLBACK(on_Enviar_Mensagem), builder);

	GtkTreeView *Chat = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Chat"));
	g_signal_connect(Chat, "row-activated", G_CALLBACK(on_Chat_clicked), builder);

	GtkWidget *Apagar_Respondendo_a = GTK_WIDGET(gtk_builder_get_object(builder, "Apagar_Respondendo_a"));
	g_signal_connect(Apagar_Respondendo_a, "clicked", G_CALLBACK(on_Apagar_Respondendo_a), builder);

	GtkWidget *Sair_da_escolha_mensagem = GTK_WIDGET(gtk_builder_get_object(builder, "Sair_da_escolha_mensagem"));
	g_signal_connect(Sair_da_escolha_mensagem, "clicked", G_CALLBACK(on_Sair_da_escolha_mensagem), builder);

	GtkEntry *Pesquisar_Usuarios = (GTK_ENTRY(gtk_builder_get_object(builder, "Pesquisar_Usuarios")));
	g_signal_connect(Pesquisar_Usuarios, "search-changed", G_CALLBACK(on_Pesquisar_Usuarios), builder);

	GtkWidget *Filtrar_não_lidas = GTK_WIDGET(gtk_builder_get_object(builder, "Filtrar_não_lidas"));
	g_signal_connect(Filtrar_não_lidas, "toggled", G_CALLBACK(on_Filtrar_não_lidas), builder);

	GtkTreeView *Iniciar_Chat_empresa = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Iniciar_Chat_empresa"));
	g_signal_connect(Iniciar_Chat_empresa, "row-activated", G_CALLBACK(on_Iniciar_Chat_empresa), builder);

	GtkWidget *Cancelar_Seleção1 = GTK_WIDGET(gtk_builder_get_object(builder, "Cancelar Seleção1"));
	g_signal_connect(Cancelar_Seleção1, "clicked", G_CALLBACK(on_Cancelar_Seleção1), builder);

	GtkTreeView *Mensagens_Recebidas = GTK_TREE_VIEW(gtk_builder_get_object(builder, "Mensagens_Recebidas"));
	g_signal_connect(Mensagens_Recebidas, "row-activated", G_CALLBACK(on_Tabela_Chamados_responder), builder);

//----------------------------------------------------------------------------Lidas Chamados e Mensagem---------------------------------------------------------------------------------------------------------------------------

	GtkWidget *Chamados_nao_lidos_filtrar = GTK_WIDGET(gtk_builder_get_object(builder, "Chamados_nao_lidos_filtrar"));
	g_signal_connect(Chamados_nao_lidos_filtrar, "toggled", G_CALLBACK(on_Chamados_nao_lidos_filtrar), builder);

	GtkWidget *Mensagens_nao_lidas_filtrar = GTK_WIDGET(gtk_builder_get_object(builder, "Mensagens_nao_lidas_filtrar"));
	g_signal_connect(Mensagens_nao_lidas_filtrar, "toggled", G_CALLBACK(on_Mensagens_nao_lidas_filtrar), builder);

	// Exibe a janela
	gtk_widget_show_all(window);

	// Inicia o loop principal do GTK
	gtk_main();


	return 0;
}
