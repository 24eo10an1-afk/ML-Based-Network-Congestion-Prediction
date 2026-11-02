# ML-Based Early Congestion Prediction and Proactive Rerouting in Computer Networks Using NS-3

## 1. Project Overview

This project explores an intelligent network congestion management system using Machine Learning and NS-3 network simulation.

It uses a Random Forest classifier to predict potential network congestion and trigger proactive rerouting of data packets through an alternative network path.

The main objective is to reduce packet loss, network delay and congestion while improving overall network performance.

## 2. Problem Statement

Network congestion occurs when the amount of data transmitted through a network exceeds the available capacity of a network path.

It can result in:
- Increased packet delay
- Packet loss
- Reduced throughput
- Poor network performance

This project explores a machine-learning-based approach to predict congestion and redirect traffic through an alternative route.

## 3. System Architecture

The simulated network consists of:

- 1 Source Node
- 10 Intermediate Routers
- 1 Destination Node
- 2 Alternative Network Paths

### Network Paths

| Parameter | Path A | Path B |
|---|---|---|
| Bottleneck Bandwidth | 2 Mbps | 8 Mbps |
| Purpose | Primary Path | Alternative Path |

## 4. Working Methodology

1. NS-3 creates the network topology and generates data packets.
2. Network parameters such as throughput, delay, packet loss and traffic information are monitored.
3. Network data is processed by a Random Forest machine learning model.
4. The model predicts whether congestion is likely to occur.
5. When congestion is predicted, the routing mechanism triggers rerouting from Path A to Path B.
6. Network performance is visualized using a web dashboard and NetAnim.

## 5. Machine Learning Implementation

The project uses the Random Forest Classifier for congestion prediction.

Two trained models are included:

- `congestion_model.pkl` – Existing congestion prediction model.
- `early_congestion_model.pkl` – Early congestion prediction model.

The early prediction model uses nine input features:

- Traffic Rate
- Early Packets Sent
- Early Packets Received
- Early Packets Lost
- Early Delay
- Early Jitter
- Early Throughput
- Early Queue
- Utilization

The training dataset is stored in `early_ml_dataset.csv`.

## 6. Technologies Used

| Technology | Purpose |
|---|---|
| NS-3 | Network Simulation |
| C++ | Network Topology and Routing |
| Python | ML Implementation |
| Random Forest | Congestion Prediction |
| Pandas | Dataset Processing |
| Scikit-learn | Machine Learning |
| Joblib | Model Storage |
| HTML, CSS, JavaScript | Web Dashboard |
| NetAnim | Network Visualization |
| CSV | Dataset Storage |

## 7. Project Structure

| File | Description |
|---|---|
| `multipath10.cc` | NS-3 network simulation |
| `train_early_model.py` | Trains the early congestion model |
| `ml_predict.py` | Performs congestion prediction |
| `ml_decision.py` | Generates congestion and rerouting decisions |
| `early_ml_dataset.csv` | ML training dataset |
| `congestion_model.pkl` | Existing trained model |
| `early_congestion_model.pkl` | Early prediction model |
| `dashboard.html` | Web monitoring dashboard |
| `dashboard_server.py` | Dashboard server |
| `requirements.txt` | Python dependencies |

## 8. Installation and Setup

### Step 1: Clone the Repository

```bash
git clone https://github.com/24eo10an1-afk/ML-Based-Network-Congestion-Prediction.git
```

### Step 2: Open Project Directory

```bash
cd ML-Based-Network-Congestion-Prediction
```

### Step 3: Install Python Dependencies

```bash
pip install -r requirements.txt
```

### Step 4: Set Up NS-3

Install and configure NS-3.48 separately.

Copy `multipath10.cc` into the NS-3 `scratch` directory.

### Step 5: Build and Run the Simulation

From the NS-3.48 directory:

```bash
./ns3 build scratch/multipath10
./ns3 run scratch/multipath10
```

### Step 6: Launch the Dashboard

Run the dashboard server from the directory containing `dashboard_server.py`:

```bash
python3 dashboard_server.py
```

Open the dashboard in your browser:

```text
http://localhost:8000
```

## 9. Applications

Potential application areas include:

- Telecommunication Networks
- Cloud Computing and Data Centres
- Internet of Things (IoT)
- Online Gaming
- Video Streaming
- Smart City Networks

## 10. Current Limitations

- The system is currently evaluated in an NS-3 simulation environment.
- The current topology uses two predefined network paths.
- Prediction performance depends on the quality and diversity of the training dataset.
- Real-world deployment and large-scale network testing have not yet been established.

## 11. Future Scope

- Support for larger network top
