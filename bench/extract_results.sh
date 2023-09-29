#!/bin/bash

for m in Insert Find Erase; do
    for i in STL Perroht STL-Metall Perroht-Metall; do
        for s in {25..30}; do
            grep -w "${m}-${i}" log-ruby-s${s}.txt
        done
    done
done