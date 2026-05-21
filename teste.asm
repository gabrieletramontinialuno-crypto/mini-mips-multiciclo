addi $r2, $r0, -5
addi $r1, $r0, 3
add $r3, $r2, $r1
add $r3, $r2, $r1
addi $r4, $r0, 10
sw $r2, 0($r0)
lw $r1, 4($r0)
beq $r0, $r2, 8
beq $r0, $r5, 1
beq $r3, $r2, 10
j 1
