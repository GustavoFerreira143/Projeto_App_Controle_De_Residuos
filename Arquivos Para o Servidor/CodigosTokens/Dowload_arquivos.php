<?php
require 'vendor/autoload.php';
use \Firebase\JWT\JWT;

$secret_key = "Gustavo";  // Mesma chave secreta usada para gerar o token
$algorithm = 'HS256'; // Algoritmo de hash

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    // Captura o token
    $headers = apache_request_headers();

    // Verifica diferentes formas de recuperar o cabeçalho Authorization
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
        // Decodifica o token JWT
        $decoded = JWT::decode($token, new \Firebase\JWT\Key($secret_key, $algorithm));
        $valor = $_GET['file'];
        // Verifica se o arquivo existe
        $file_path= realpath('../../Relatorios/' . $valor); // Altere o caminho conforme necessário


        if (file_exists($file_path)) {
            // Configurações de headers para o download do arquivo
            header('Content-Description: File Transfer');
            header('Content-Type: application/octet-stream');
            header('Content-Disposition: attachment; filename="'.basename($file_path).'"');
            header('Expires: 0');
            header('Cache-Control: must-revalidate');
            header('Pragma: public');
            header('Content-Length: ' . filesize($file_path));
            
            // Envia o arquivo para o usuário
            readfile($file_path);
            exit;
        } else {
            echo json_encode(['status' => 'Arquivo não encontrado']);
            exit;
        }
    } catch (Exception $e) {
        echo json_encode(['status' => 'Token inválido', 'message' => $e->getMessage()]);
    }
} else {
    echo json_encode(['status' => 'Método não permitido']);
}
?>
