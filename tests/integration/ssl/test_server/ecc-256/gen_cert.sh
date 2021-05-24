  #!/bin/sh
  openssl ecparam -name secp256r1 -genkey -noout -out ca-private-key.pem
  openssl req -new -sha256 -key ca-private-key.pem -out ca-csr.pem -subj "/C=SA/ST=Czechia/L=Prague/O=Prusa test CA"
  openssl x509 -req -signkey ca-private-key.pem -in ca-csr.pem -out ca-cer.pem -days 30
  openssl ecparam -name secp256r1 -genkey -noout -out server-prk.pem
  openssl req -new -sha256 -key server-prk.pem -out server-csr.pem -subj "/C=SA/ST=Czechia/L=Prague/O=Prusa test CA Server/CN=192.168.88.189"
  openssl req -in server-csr.pem -noout -text
  openssl x509 -req -sha256 -in server-csr.pem -CA ca-cer.pem -CAkey ca-private-key.pem -CAcreateserial -out server-cer.pem -days 30
   openssl x509 -in server-cer.pem -noout -text
