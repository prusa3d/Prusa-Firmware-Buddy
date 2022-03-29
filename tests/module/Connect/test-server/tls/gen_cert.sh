  #!/bin/sh
  openssl ecparam -name secp256r1 -genkey -noout -out server-key.pem
  openssl ecparam -name secp256r1 -genkey -noout -out ca-key.pem

  openssl req -new -sha256 -key ca-key.pem -out ca.csr -subj "/C=SA/ST=Czechia/L=Prague/O=Test CA"
  openssl x509 -req -signkey ca-key.pem -in ca.csr -out ca-cert.pem -days 365

  openssl req -new -sha256 -key server-key.pem -out server.csr -subj "/C=SA/ST=Czechia/L=Prague/O=Test Server/CN=192.168.88.189"
  openssl x509 -req -sha256 -in server.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial -out server-cert.pem -days 365
