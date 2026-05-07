cd /home/utnso/tp-2025-2c-noSabemosC/query_control

for i in $(seq 1 25); do
  ./bin/query_control Query.config AGING_1 20 &
  ./bin/query_control Query.config AGING_2 20 &
  ./bin/query_control Query.config AGING_3 20 &
  ./bin/query_control Query.config AGING_4 20 &
done

wait
echo "Terminaron todas las queries AGING"
