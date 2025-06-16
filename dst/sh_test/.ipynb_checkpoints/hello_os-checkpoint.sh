#!/bin/bash
sed -n 8P $1 >> $2
sed -n 32P $1 >> $2
sed -n 128p $1 >> $2
sed -n 512P $1 >> $2
sed -n 1024P $1 >> $2