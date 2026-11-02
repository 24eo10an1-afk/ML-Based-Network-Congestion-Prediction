# ML-Based Early Congestion Prediction and Proactive Rerouting in Computer Networks Using NS-3

## 1. Project Overview

This project focuses on predicting network congestion using Machine Learning and proactively rerouting data packets through an alternative network path.

It uses NS-3 for network simulation and a Random Forest classifier for congestion prediction. The main objective is to improve network performance by identifying congestion conditions and triggering an alternative routing decision.

## 2. Problem Statement

Network congestion occurs when a large amount of data is transmitted through a network path beyond its available capacity. This can cause increased delay, packet loss and reduced throughput.

This project explores an intelligent approach to predict congestion and redirect traffic from a congested path to an alternative route.

## 3. System Architecture

The simulated network consists of:
- One source node
- Ten routers
- One destination node
- Two alternative network paths

**Path A:** 2 Mbps bottleneck bandwidth.

**Path B:** 8 Mbps bottleneck bandwidth.

## 4. Working Methodology

1. NS-3 creates the network topology and generates data packets.
2. Network performance parameters such as throughput, delay and packet loss are monitored.
3. Network data is used by a Random Forest machine learning model to predict congestion.
4. When congestion is predicted, the routing mechanism triggers rerouting from Path A to Path B.
5. Network performance is visualized using a web dashboard and NetAnim.

## 5. Technologies Used

- NS-3
- C++
- Python
- Machine Learning
- Random Forest Classifier
- HTML, CSS and JavaScript
- NetAnim
- CSV

## 6. Project Files

| File | Description |
|---|---|
| multipath10.cc | NS-3 network simulation |
| dashboard.html | Web-based monitoring dashboard |
| dashboard_server.py | Dashboard server |

## 7. Applications

- Telecommunication networks
- Cloud computing and data centres
- Internet of Things (IoT)
- Online gaming
- Video streaming
- Smart city networks

## 8. Future Scope

- Support for larger network topologies.
- Dynamic selection among multiple alternative paths.
- Improved congestion prediction using more training data.
- Real-time network monitoring and testing.
- Advanced load balancing techniques.

## 9. Project Objective

To explore how Machine Learning can support intelligent network management through early congestion prediction and proactive rerouting.
