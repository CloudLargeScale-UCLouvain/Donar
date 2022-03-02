terraform {
  required_providers {
    scaleway = {
      source = "scaleway/scaleway"
      version = "2.0.0-rc1"
    }
  }
}

provider "scaleway" {
  zone = "fr-par-1"
  region = "fr-par"
  project_id = "1915f95a-dea4-4222-8e6b-a90b6125d1f4"
}



/**********************
 * TOR EXIT
 **********************/
resource "scaleway_instance_ip" "torexit_ip" {}
resource "scaleway_instance_server" "torexit" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torexit"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
systemctl start torexithelper.service
sleep 5
for i in $(seq 1 8); do echo start $i ; systemctl start torexit@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torexit_ip.id
}

resource "scaleway_instance_ip" "torexitna_ip" {}
resource "scaleway_instance_server" "torexitna" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torexitna"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
systemctl start torexitnahelper.service
sleep 5
for i in $(seq 1 8); do echo start $i ; systemctl start torexitna@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torexitna_ip.id
}


/**********************
 * TOR HIDDEN SERVICE
 **********************/
resource "scaleway_instance_ip" "torhs_ip" {}
resource "scaleway_instance_server" "torhs" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torhs"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torhs@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torhs_ip.id
}

resource "scaleway_instance_ip" "torhsna_ip" {}
resource "scaleway_instance_server" "torhsna" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torhsna"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torhsna@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torhsna_ip.id
}

