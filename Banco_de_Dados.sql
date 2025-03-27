/* Código MySql para desenvolver o banco por completo da aplicação  ele deve ser feito por partes para se evitar erros, eu recomendo que os comandos sejam realizados em partes para evitar bugs ou falhas 
pois a tabelas que dependem de criação de outras assim podem ocorrer erros no momento de instalação*/



CREATE DATABASE projeto_ambiental;

/*

Selecione o banco de dados criado e insira as tabelas a seguir

CREATE TABLE usuarios_admins(
    id_usuario_adm int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    nome varchar(255) NOT NULL,
    nome_usuario varchar(255) NOT NULL,
    senha varchar(255) NOT NULL,
    cpf varchar(255) NOT NULL,
    email varchar(255) NOT NULL,
    data_de_criacao DATETIME NOT NULL,
    usuario_criador varchar(255) NOT NULL  
)
;
CREATE TABLE usuarios_empresas(
    id_usuario_empresa int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    nome_usuario_empresa varchar(255) NOT NULL,
    senha_usuario_empresa varchar(255) NOT NULL,
    nome_da_empresa varchar(255) NOT NULL,
    cnpj varchar(255) NOT NULL,
    razao_social varchar(255) NOT NULL,
    nome_fantasia varchar(255),
    telefone varchar(15),
    rua varchar(255) NOT NULL,
    numero_empresa int(5) NOT NULL,
    bairro varchar(255) NOT NULL,
    cidade varchar(255) NOT NULL,
    estado varchar(2) NOT NULL,
    cep int(8) NOT NULL,
    email_empresa varchar(255) NOT NULL,
    data_de_cadastro DATETIME NOT NULL
)
;

*/


/*  Após Rodar os dados anteriores libere para rodar as demais tabelas */



/*
CREATE TABLE relatorio_valores(
    id_relatorio int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    nome_arquivo varchar(255) NOT NULL,
    chave int NOT NULL,
    quantidade_residuos varchar(255) NOT NULL,
    valor_custo varchar(255) NOT NULL,
    empresa varchar(255) NOT NULL,
    data DATETIME NOT NULL,
    FOREIGN KEY(chave) REFERENCES usuarios_empresas(id_usuario_empresa)
)
;

CREATE TABLE mensagens(
    id_mensagem int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    titulo_mensagem varchar(255) NOT NULL,
    corpo_mensagem text NOT NULL,
    empresa_destino varchar(255) NOT NULL,
    usuario_criador varchar(255) NOT NULL,
    data_de_criacao DATETIME NOT NULL,
    data_de_modificacao varchar(255) ,
    usuario_modificador varchar(255),
    lido boolean Not NULL DEFAULT FALSE
)
;
CREATE TABLE chamados(
    id_chamado int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    titulo_chamado varchar(255) NOT NULL,
    corpo_chamado text NOT NULL,
    nome_empresa varchar(255) NOT NULL,
    data_criacao DATETIME NOT NULL,
    data_de_modificacao varchar(255),
    usuario_modificador varchar(255),
    lido boolean Not NULL DEFAULT FALSE
)
;  

CREATE TABLE bate_papo(
    id int NOT NULL AUTO_INCREMENT PRIMARY KEY,
    remetente_id INT NOT NULL,
    destinatario_id INT NOT NULL,
    conteudo TEXT NOT NULL,
    hora_envio DATETIME,
    mensagem_referente TEXT,
    lida boolean NOT NULL DEFAULT FALSE,
    nome_do_remetente varchar(255) NOT NULL,
    nome_do_destinatario varchar(255) NOT NULL
)

*/

/* Adicionar o Primeiro Usuário

INSERT INTO usuarios_admins(nome,nome_usuario,senha,cpf,email,data_de_criacao,usuario_criador) VALUES("admin", "admin" , "$argon2id$v=19$m=65536,t=2,p=1$VtQBfy59CQYrdDBCJDx4FA$tYGp+geHXwSqm9QWJ8wcip6WrW1wD7GPXmLxoFqLLSE", "00000000000","admin@admin", "2024-11-01 00:00:00","admin");
*/