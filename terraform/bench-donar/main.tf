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
resource "scaleway_instance_ip" "torhsopt_ip" {}
resource "scaleway_instance_server" "torhsopt" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torhsopt"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torhsopt@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torhsopt_ip.id
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

/**********************
 * TORFONE
 **********************/
resource "scaleway_instance_ip" "torfone_ip" {}
resource "scaleway_instance_server" "torfone" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torfone"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torfone@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torfone_ip.id
}
resource "scaleway_instance_ip" "torfoneopt_ip" {}
resource "scaleway_instance_server" "torfoneopt" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torfoneopt"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torfoneopt@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torfoneopt_ip.id
}
resource "scaleway_instance_ip" "torfonena_ip" {}
resource "scaleway_instance_server" "torfonena" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "torfonena"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start torfonena@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.torfonena_ip.id
}

/**********************
 * DONAR ALT
 **********************/
resource "scaleway_instance_ip" "donaralt_ip" {}
resource "scaleway_instance_server" "donaralt" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donaralt"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donaralt@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donaralt_ip.id
}
resource "scaleway_instance_ip" "donaraltopt_ip" {}
resource "scaleway_instance_server" "donaraltopt" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donaraltopt"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donaraltopt@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donaraltopt_ip.id
}
resource "scaleway_instance_ip" "donaraltna_ip" {}
resource "scaleway_instance_server" "donaraltna" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donaraltna"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donaraltna@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donaraltna_ip.id
}


/**********************
 * DONAR DUP
 **********************/
resource "scaleway_instance_ip" "donardup_ip" {}
resource "scaleway_instance_server" "donardup" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donardup"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donardup@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donardup_ip.id
}
resource "scaleway_instance_ip" "donardupopt_ip" {}
resource "scaleway_instance_server" "donardupopt" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donardupopt"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donardupopt@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donardupopt_ip.id
}
resource "scaleway_instance_ip" "donardupna_ip" {}
resource "scaleway_instance_server" "donardupna" {
  type  = "DEV1-L"
  image = "ubuntu_focal"
  name = "donardupna"
  cloud_init = <<-EOT
#!/bin/bash
wget https://gitlab.inria.fr/qdufour/donar/-/raw/master/scripts/2021/install.sh?inline=false -O - | bash
for i in $(seq 1 8); do echo start $i ; systemctl start donardupna@$i ; sleep 60 ; done
EOT
  ip_id = scaleway_instance_ip.donardupna_ip.id
}
