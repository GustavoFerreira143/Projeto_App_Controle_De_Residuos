<?php
require 'vendor/autoload.php';
use \Firebase\JWT\JWT;

$secret_key = "Gustavo";  // Defina sua chave secreta
$algorithm = 'HS256';     // Algoritmo de hash

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Payload de dados do token
    $payload = [
        'user' => 'admin',
        'exp' => time() + 20 // Expira em 1 hora
    ];

    // Gera o token JWT
    $jwt = JWT::encode($payload, $secret_key, $algorithm);

    // Retorna o token no formato JSON
    echo json_encode(['token' => $jwt]);
}
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
        echo json_encode(['status' => 'Token válido', 'data' => $decoded]);
    } catch (Exception $e) {
        echo json_encode(['status' => 'Token inválido', 'message' => $e->getMessage()]);
    }
}
?>
