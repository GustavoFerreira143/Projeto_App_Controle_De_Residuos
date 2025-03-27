<?php
require 'vendor/autoload.php';
use \Firebase\JWT\JWT;

$secret_key = "Gustavo";
$algorithm = 'HS256';

$dsn = 'mysql:host=127.0.0.1;dbname=projeto_ambiental;charset=utf8mb4'; // Adicionando charset utf8mb4
$username = 'root';
$password = '';

try {
    $pdo = new PDO($dsn, $username, $password);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $pdo->exec("SET NAMES 'utf8mb4'"); // Configura a conexão para usar utf8mb4
} catch (PDOException $e) {
    echo json_encode(['status' => 'Erro na conexão com o banco de dados', 'message' => $e->getMessage()]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    header('Content-Type: application/json; charset=utf-8'); // Define o cabeçalho de resposta

    $headers = apache_request_headers();
    $token = null;

    if (isset($headers['Authorization'])) {
        $token = str_replace('Bearer ', '', $headers['Authorization']);
    } elseif (isset($_SERVER['HTTP_AUTHORIZATION'])) {
        $token = str_replace('Bearer ', '', $_SERVER['HTTP_AUTHORIZATION']);
    } elseif (isset($_SERVER['REDIRECT_HTTP_AUTHORIZATION'])) {
        $token = str_replace('Bearer ', '', $_SERVER['REDIRECT_HTTP_AUTHORIZATION']);
    } else {
        echo json_encode(['status' => 'Token não fornecido']);
        exit;
    }

    try {
        $decoded = JWT::decode($token, new \Firebase\JWT\Key($secret_key, $algorithm));
        $input_data = file_get_contents("php://input");

        // Debug: Verificar o que está sendo recebido
        error_log("Input data: " . $input_data); // Log do input

        // Extraímos o valor antes do primeiro `*`
        $id_relatorio = strtok($input_data, '*');

        // Consulta ao banco de dados para obter o nome do arquivo
        $stmt = $pdo->prepare("SELECT nome_arquivo FROM relatorio_valores WHERE id_relatorio = :id_relatorio");
        $stmt->bindParam(':id_relatorio', $id_relatorio, PDO::PARAM_STR);
        $stmt->execute();

        $nome_arquivo_db = $stmt->fetchColumn();
        $nome_arquivo = $nome_arquivo_db ? $nome_arquivo_db : "arquivo_padrao.txt";

        // Definindo o diretório e verificando se ele existe
        $diretorio_destino = __DIR__ . "/../Relatorios/";
        if (!is_dir($diretorio_destino)) {
            mkdir($diretorio_destino, 0755, true);
        }

        // Formata o nome do arquivo para evitar caracteres inválidos
        $nome_arquivo = str_replace([':', ' '], ['-', '_'], $nome_arquivo);
        $caminho_arquivo = $diretorio_destino . $nome_arquivo;

        // Log do caminho do arquivo para verificar
        error_log("Caminho do arquivo: " . $caminho_arquivo);

        // Abre o arquivo para gravação
        $output_file = fopen($caminho_arquivo, "wb");
        if ($output_file === false) {
            echo json_encode(['status' => 'Falha ao abrir o arquivo para gravação']);
            exit;
        }

        // Remove a parte antes do `*` e o próprio `*` do conteúdo
        $conteudo_para_salvar = strstr($input_data, '*');
        if ($conteudo_para_salvar !== false) {
            $conteudo_para_salvar = substr($conteudo_para_salvar, 1); // Retira o primeiro caractere `*`
        } else {
            $conteudo_para_salvar = ''; // Se não houver `*`, o conteúdo será vazio
        }

        // Debug: Verificar o conteúdo a ser salvo
        error_log("Conteúdo a ser salvo: " . $conteudo_para_salvar); // Log do conteúdo a ser salvo

        // Escreve o conteúdo no arquivo
        fwrite($output_file, $conteudo_para_salvar);
        fclose($output_file);

        // Lê o conteúdo salvo para incluir na resposta
        $conteudo_salvo = file_get_contents($caminho_arquivo);

        // Debug: Verificar o conteúdo salvo
        error_log("Conteúdo salvo no arquivo: " . $conteudo_salvo);

        echo json_encode([
            'status' => 'Arquivo enviado com sucesso',
            'path' => $caminho_arquivo,
            'conteudo_salvo' => $conteudo_salvo
        ]);
    } catch (Exception $e) {
        echo json_encode(['status' => 'Token inválido', 'message' => $e->getMessage()]);
        exit;
    }
} else {
    echo json_encode(['status' => 'Método não permitido']);
    exit;
}
