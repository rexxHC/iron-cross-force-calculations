#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int gender;
  float height;
  float massBody;
} athlete;

typedef struct {
  float L_arm;      // length of arm
  float L_upperArm; // length of upper arm
  float L_forearm;  // length of forearm
  float L_hand;     // length of hand
  float L_cg;       // center of gravity of the arm
} length_arm;

typedef struct {
  float L_pec;      // pectoralis insertion length
  float A_pec;      // pectoralis insertion angle
  float L_lat;      // latisimus insertion length
  float A_lat;      // latissimus insertion angle
  float PCSA_ratio; // Physiological cross sectional area ratio (Area_lat /
                    // Area_pec)
} muscle_insertion;

typedef struct {
  float M_upperArm; // mass from elbow to shoulder joint
  float M_foreArm;  // mass from elbow to wrists
  float M_hand;     // mass from wrists to fingertips
  float M_arm;      // mass of the whole arm
} mass_arm;

int getGender() {
  while (1) {
    int n;
    printf("Select Gender\n1.Male\n2.Female\n->");
    scanf("%d", &n);
    if (n == 1) {
      return n;
    } else if (n == 2) {
      return n;
    } else {
      printf("invalid choice\n");
    }
  }
}

float getHeight() {
  float n;
  printf("Enter Height (meters): ");
  scanf("%f", &n);
  return n;
}

float getWeight() {
  float n;
  printf("Enter Weight (kgs): ");
  scanf("%f", &n);
  return n;
}

float upperArm_M(int gender, float weight, float height) {
  height = height * 100; // conversion to cm
  float b0, b1, b2;

  if (gender == 1) {
    b0 = -5.40;
    b1 = 0.0336;
    b2 = 0.0242;
  } else {
    b0 = -3.90;
    b1 = 0.0363;
    b2 = 0.0162;
  }

  float mass_upperArm = b0 + (b1 * weight) + (b2 * height);

  return mass_upperArm;
}

float foreArm_M(int gender, float weight, float height) {
  height = height * 100; // conversion to cm
  float b0, b1, b2;

  if (gender == 1) {
    b0 = 0.86;
    b1 = 0.0123;
    b2 = -0.0051;
  } else {
    b0 = 0.28;
    b1 = 0.0095;
    b2 = -0.0011;
  }

  float mass_foreArm = b0 + (b1 * weight) + (b2 * height); // Zatsiorsky

  return mass_foreArm;
}

float hand_M(int gender, float weight, float height) {
  height = height * 100; // conversion to cm
  float b0, b1, b2;

  if (gender == 1) {
    b0 = -0.27;
    b1 = 0.0055;
    b2 = 0.0023;
  } else {
    b0 = -0.24;
    b1 = 0.0039;
    b2 = 0.0028;
  }

  float mass_hand = b0 + (b1 * weight) + (b2 * height);

  return mass_hand;
}

float upperArm_L(int gender, float height) {
  float upperarm_length;
  if (gender == 1) {
    upperarm_length = height * 0.186;
  } else {
    upperarm_length = height * 0.182;
  }
  return upperarm_length;
}

float foreArm_L(int gender, float height) {
  float forearm_length;
  if (gender == 1) {
    forearm_length = height * 0.146;
  } else {
    forearm_length = height * 0.143;
  }
  return forearm_length;
}

float hand_L(int gender, float height) {
  float hand_length;
  if (gender == 1) {
    hand_length = height * 0.108;
  } else {
    hand_length = height * 0.106;
  }
  return hand_length;
}

float cog_Arm(float mass_arm, float mass_upperArm, float mass_foreArm,
              float mass_hand, float length_upperArm, float length_forearm,
              float length_hand) {
  float x1, x2, x3;
  x1 = length_upperArm * 0.436;
  x2 = length_upperArm + (length_forearm * 0.430);
  x3 = length_upperArm + length_forearm + (length_hand * 0.506);

  float cog_arm =
      ((mass_upperArm * x1) + (mass_foreArm * x2) + (mass_hand * x3)) /
      mass_arm;
  return cog_arm;
}

float pec_ins(float L_upper) {
  float len = L_upper * 0.175;
  return len;
}

float lat_ins(float L_upper) {
  float len = L_upper * 0.140;
  return len;
}

int main() {
  athlete *Athlete = (athlete *)malloc(sizeof(athlete));
  if (Athlete == NULL)
    return 1;

  Athlete->gender = getGender(); // 1Male 2Female
  Athlete->height = getHeight();
  Athlete->massBody = getWeight();

  mass_arm *athlete_arm = (mass_arm *)malloc(sizeof(mass_arm));
  if (athlete_arm == NULL)
    return 1;

  athlete_arm->M_upperArm =
      upperArm_M(Athlete->gender, Athlete->massBody, Athlete->height);
  athlete_arm->M_foreArm =
      foreArm_M(Athlete->gender, Athlete->massBody, Athlete->height);
  athlete_arm->M_hand =
      hand_M(Athlete->gender, Athlete->massBody, Athlete->height);

  athlete_arm->M_arm =
      athlete_arm->M_upperArm + athlete_arm->M_foreArm + athlete_arm->M_hand;

  length_arm *arm_length = (length_arm *)malloc(sizeof(length_arm));
  if (arm_length == NULL)
    return 1;

  arm_length->L_upperArm = upperArm_L(Athlete->gender, Athlete->height);
  arm_length->L_forearm = foreArm_L(Athlete->gender, Athlete->height);
  arm_length->L_hand = hand_L(Athlete->gender, Athlete->height);

  arm_length->L_arm =
      arm_length->L_upperArm + arm_length->L_forearm + arm_length->L_hand;

  arm_length->L_cg = cog_Arm(athlete_arm->M_arm, athlete_arm->M_upperArm,
                             athlete_arm->M_foreArm, athlete_arm->M_hand,
                             arm_length->L_upperArm, arm_length->L_forearm,
                             arm_length->L_hand);

  muscle_insertion *insertions =
      (muscle_insertion *)malloc(sizeof(muscle_insertion));
  if (insertions == NULL)
    return 1;

  insertions->L_pec = pec_ins(arm_length->L_upperArm);
  insertions->L_lat = lat_ins(arm_length->L_upperArm);
  insertions->A_pec = 20 * (M_PI / 180.0);
  insertions->A_lat = 15 * (M_PI / 180.0);
  insertions->PCSA_ratio = 0.73;

  // --- PHYSICS CALCULATIONS ---
  float arm_derivation = 0;
  float g = 9.81;

  // Acting forces
  float ring_force = (Athlete->massBody * g) / 2;
  float arm_weight = athlete_arm->M_arm * g;

  // External torque
  float ring_torque = ring_force * arm_length->L_arm * cos(arm_derivation);
  float arm_weight_torque = arm_weight * arm_length->L_cg * cos(arm_derivation);
  float total_external = ring_torque - arm_weight_torque;

  // Pectoralis Force
  float F_pec =
      total_external /
      ((insertions->L_pec * sin(insertions->A_pec)) +
       (insertions->PCSA_ratio * (insertions->L_lat * sin(insertions->A_lat))));

  // Latissimus Force
  float F_lat = F_pec * insertions->PCSA_ratio;

  // Deltoid (Joint Reaction) Force
  float joint_horizontal =
      (F_pec * cos(insertions->A_pec)) + (F_lat * cos(insertions->A_lat));

  float joint_vertical = ring_force - arm_weight -
                         (F_pec * sin(insertions->A_pec)) -
                         (F_lat * sin(insertions->A_lat));

  float deltoid_total = sqrt(pow(joint_horizontal, 2) + pow(joint_vertical, 2));

  printf("Deltoid Reaction Force: %f N\n", deltoid_total);

  free(Athlete);
  free(athlete_arm);
  free(arm_length);
  free(insertions);

  return 0;
}
