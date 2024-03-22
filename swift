//
//  ViewController.swift
//  SmartSoda
//
//  Created by Antoine MG on 13.03.2024.
//

import UIKit
import CoreBluetooth

class ViewController: UIViewController, CBCentralManagerDelegate, CBPeripheralDelegate {
    @IBOutlet weak var tableauDiscoveredPeripherals: UITableView!
    @IBOutlet weak var tableauDiscoveredPeripheralsCell1: UITableView!
    
    var centralManager: CBCentralManager!
    var discoveredPeripherals: [CBPeripheral] = []
    var peripheral: CBPeripheral?
    // Déclarer la propriété characteristic
    var writableCharacteristic: CBCharacteristic?
    
    @IBOutlet var connexionStatusLabel: UIView!
    @IBOutlet weak var sliderLiq1Val: UISlider!
    @IBOutlet weak var quantityLiq1: UILabel!
    @IBOutlet weak var sliderLiq2Val: UISlider!
    @IBOutlet weak var quantityLiq2: UILabel!
    @IBOutlet var buttonSearchAppareilClick: UIView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
        centralManager = CBCentralManager(delegate: self, queue: nil)
        updateLabelLiq1()
        updateLabelLiq2()
        tableauDiscoveredPeripherals.delegate = self
        tableauDiscoveredPeripherals.dataSource = self
        // Enregistrer une cellule personnalisée
        tableauDiscoveredPeripherals.register(UITableViewCell.self, forCellReuseIdentifier: "Cell")
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
            if central.state == .poweredOn {
                // Commence à rechercher les périphériques Bluetooth lorsque Bluetooth est allumé
                let serviceUUIDs: [CBUUID] = [CBUUID(string: "0000180A-0000-1000-8000-00805F9B34FB")]

                centralManager.scanForPeripherals(withServices: serviceUUIDs, options: nil)
            } else {
                // Gérer le cas où Bluetooth n'est pas disponible
                print("Bluetooth not available.")
            }
        }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        // Vérifie si le périphérique est déjà dans la liste
            if !discoveredPeripherals.contains(peripheral) {
                // Ajoute le périphérique découvert au tableau
                discoveredPeripherals.append(peripheral)
                // Rafraîchit la tableview pour refléter les périphériques mis à jour
                reloadTableView()
            }
        }
    
    func reloadTableView() {
            DispatchQueue.main.async {
                self.tableauDiscoveredPeripherals.reloadData()
            }
        }
    
    // Fonction delegate appelée lors de la découverte des services du périphérique
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else {
            return
        }

        // Parcourir les services découverts
        for service in services {
            // Découvrir les caractéristiques du service
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }

    // Fonction delegate appelée lors de la découverte des caractéristiques d'un service
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else {
            return
        }

        // Parcourir les caractéristiques découvertes
        for characteristic in characteristics {
            // Vérifier si la caractéristique est une caractéristique à écrire
            if characteristic.properties.contains(.write) || characteristic.properties.contains(.writeWithoutResponse) {
                // Stocker la caractéristique à écrire
                writableCharacteristic = characteristic
                break
            }
        }
    }
    
    @IBAction func boutonRechercheAppareilBT(_ sender: UIButton) {
        centralManager.scanForPeripherals(withServices: nil, options: nil)
    }
    
    @IBAction func SliderLiq1Change(_ sender: UISlider) {
        // When the slider value changes, update the label
        updateLabelLiq1()
    }
    
    @IBAction func SliderLiq2Change(_ sender: UISlider) {
        // When the slider value changes, update the label
        updateLabelLiq2()
    }
    
    @IBAction func LaunchButton(_ sender: UIButton) {
        guard let peripheral = self.peripheral, let characteristic = self.writableCharacteristic else {
                print("Aucun périphérique connecté ou aucune caractéristique à écrire.")
                return
            }

            // Message à envoyer
            let message = "Hello, Bluetooth device!"

            // Convertir le message en Data
            if let data = message.data(using: .utf8) {
                // Envoyer les données au périphérique
                peripheral.writeValue(data, for: characteristic, type: .withResponse)
                print("Message envoyé avec succès.")
            } else {
                print("Erreur lors de la conversion du message en données.")
            }
        
    }
    
    
    
    func updateLabelLiq1() {
        // Update the label text with the current value of the slider
        // Obtenir la valeur entière à partir du slider
        let sliderValue = Int(sliderLiq1Val.value)
        
        // Concaténer avec "mL" à la fin
        quantityLiq1.text = "\(sliderValue) mL"
    }
    
    func updateLabelLiq2() {
        // Update the label text with the current value of the slider
        // Obtenir la valeur entière à partir du slider
        let sliderValue = Int(sliderLiq2Val.value)
        
        // Concaténer avec "mL" à la fin
        quantityLiq2.text = "\(sliderValue) mL"
    }
    
}

// Extension pour UITableViewDataSource et UITableViewDelegate
extension ViewController: UITableViewDataSource, UITableViewDelegate {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return discoveredPeripherals.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
        let peripheral = discoveredPeripherals[indexPath.row]
        cell.textLabel?.text = peripheral.name ?? "Unknown"
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
            let selectedPeripheral = discoveredPeripherals[indexPath.row]
            // Connectez-vous au périphérique sélectionné
            centralManager.connect(selectedPeripheral, options: nil)
            // Désélectionnez la cellule pour indiquer visuellement que la sélection a été effectuée
            tableView.deselectRow(at: indexPath, animated: true)
    }
}

//CBCentralManagerDelegate
extension ViewController {
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Connected to peripheral: \(peripheral)")
            if let connectedPeripheral = self.peripheral {
                connectedPeripheral.delegate = self // Assurez-vous que la propriété delegate du périphérique est configurée
                connectedPeripheral.discoverServices(nil) // Découvrir les services du périphérique
            } else {
                print("Error: No peripheral connected.")
            }
        
    }
    
    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("Failed to connect to peripheral: \(peripheral)")
    }
}


