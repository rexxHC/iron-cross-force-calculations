#include <stdio.h>

typedef struct {
  float height;     // meters
  float bodyWeight; // kg
  float armLength;  // meters
  int gender;
} athlete;

float getHeight() {
  int n;
  float inputValue;

  while (1) {
    printf("Select Format for Height Entry\n");
    printf("1. Imperial (feet)\n");
    printf("2. Metric (meters)\n");
    scanf("%d", &n);

    switch (n) {
    case 1:
      printf("Enter height (feet): ");
      scanf("%f", &inputValue);
      return inputValue * 0.3048;
    case 2:
      printf("Enter height (meters): ");
      scanf("%f", &inputValue);
      return inputValue;
    default:
      printf("Invalid option, please try again.\n\n");
      break;
    }
  }
}

float getWeight() {
  int n;
  float weight;

  while (1) {
    printf("Select Format for Weight Entry\n");
    printf("1. Imperial (lbs)\n");
    printf("2. Metric (kgs)\n");

    scanf("%d", &n);

    switch (n) {
    case 1:
      printf("Enter weight (lbs): ");
      scanf("%f", &weight);
      return weight * 0.45359237;
    case 2:
      printf("Enter weight (kgs): ");
      scanf("%f", &weight);
      return weight;
    default:
      printf("Invalid option, please try again.\n\n");
      break;
    }
  }
}

int getGender() {
  while (1) {
    int n;
    printf("Select Gender\n");
    printf("1. Male\n");
    printf("2. Female\n");
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

void getSkill() {
  int levelChoice, exerciseChoice;

  while (1) {
    printf("\nSelect Difficulty Level:\n");
    printf("1. Beginner Elements\n");
    printf("2. Intermediate Elements\n");
    printf("3. Advanced Elements\n");
    printf("4. Elite Elements\n");
    printf("Enter choice: ");
    scanf("%d", &levelChoice);

    switch (levelChoice) {
    case 1: // Beginner
      printf("1. Pushups\n2. One Arm Pushups\n3. Handstand\n4. Handstand "
             "Pushups\n");
      printf("Select exercise: ");
      scanf("%d", &exerciseChoice);
      switch (exerciseChoice) {
      case 1:
      case 2:
      case 3:
      case 4:
      default:
        printf("Invalid selection.\n");
        break;
      }
      break;

    case 2: // Intermediate
      printf("1. 90 Deg HS Pushup\n2. One Arm HS\n3. Planche\n4. Planche "
             "Pushup\n");
      printf("Select exercise: ");
      scanf("%d", &exerciseChoice);
      switch (exerciseChoice) {
      case 1:
      case 2:
      case 3:
      case 4:
      default:
        printf("Invalid selection.\n");
        break;
      }
      break;

    case 3: // Advanced
      printf("1. Maltese\n2. Iron Cross\n");
      printf("Select exercise: ");
      scanf("%d", &exerciseChoice);
      switch (exerciseChoice) {
      case 1:
      case 2:
      default:
        printf("Invalid selection.\n");
        break;
      }
      break;

    case 4: // Elite
      printf("1. Van Gelder\n2. Victorian Cross\n3. Supinated Pelican\n");
      printf("Select exercise: ");
      scanf("%d", &exerciseChoice);
      switch (exerciseChoice) {
      case 1:
      case 2:
      case 3:
      default:
        printf("Invalid selection.\n");
        break;
      }
      break;

    default:
      printf("Invalid difficulty level.\n");
      break;
    }
  }
}

int main() {
  athlete user;
  user.gender = getGender();
  user.height = getHeight();
  user.bodyWeight = getWeight();
  if (user.gender == 1) {
    user.armLength = user.height * 0.395;
  } else {
    user.armLength = user.height * 0.382;
  }
}
